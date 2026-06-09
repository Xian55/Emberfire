// LuaEngine.cpp is the SINGLE LuaBridge3 boundary in the client.
//
// It is compiled in ISOLATION (see client/CMakeLists.txt): no precompiled header, and with
// _HAS_STD_BYTE=1 (the rest of the client builds with _HAS_STD_BYTE=0 because it uses a bare `byte`
// identifier under `using namespace std`; LuaBridge3 needs std::byte). Therefore this file must NOT include
// stdafx.h or any heavy client header (they pull in `using namespace std` + the bare `byte`). It talks
// to the rest of the client only through plain primitives and a print-sink callback.
//
// The Lua VM is minilua (Lua 5.5, single-file). The core (state, allocator cap, instruction watchdog,
// sandbox env, chunk loading) is driven through the raw Lua C API; LuaBridge3 only marshals the widget /
// game-state API. Exceptions are disabled (LUABRIDGE_HAS_EXCEPTIONS 0): every untrusted call crosses a
// lua_pcall, and Lua-as-C reports errors via longjmp, not C++ throws.

#include <lua.hpp>                 // minilua (Lua 5.5) — MUST precede LuaBridge (it #errors otherwise)
#define LUABRIDGE_HAS_EXCEPTIONS 0 // bindings report errors via Lua, not C++ throw (game: no exceptions)
#include <LuaBridge.h>

#include "LuaEngine.h"
#include "LuaUI.h"           // handle boundary to the widget adapters (no client/std-byte headers)
#include "LuaEvents.h"       // single source of truth for fired event names
#include "../../Logger.h"   // header-only blog(); no std/byte usage

#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <map>
#include <set>
#include <vector>
#include <tuple>
#include <optional>
#include <ctime>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

// ---------------------------------------------------------------- impl state

// A loaded addon: its sandbox env + metadata from the .toc.
namespace
{
	struct AddonInfo
	{
		std::string name;
		std::string savedVar;                       // ## SavedVariables: <name>  (empty if none)
		std::vector<std::string> files;             // ordered .lua files
		bool enabled = true;
		bool broken  = false;
		std::optional<luabridge::LuaRef> env;       // per-addon _ENV (falls back to the shared sandbox)
	};
}

// All Lua / LuaBridge state lives here so it never leaks into the header.
struct LuaEngineImpl
{
	lua_State* L = nullptr;

	std::optional<luabridge::LuaRef> sandbox;      // the whitelisted _ENV every chunk runs under
	std::optional<luabridge::LuaRef> defaultUiEnv; // shared env for the built-in Lua default UI (ui/)

	std::function<void(const std::string&)> outputSink;  // routes print() to the console (set by host)

	std::map<int, luabridge::LuaRef> onUpdate;       // handle -> OnUpdate(self, dt)
	std::map<int, luabridge::LuaRef> onClick;        // handle -> OnClick(self, button)
	std::map<int, luabridge::LuaRef> onEnterPressed; // handle -> OnEnterPressed(self) (editbox Enter key)
	std::map<int, luabridge::LuaRef> onMouseEnter;   // handle -> OnEnter(self) (cursor entered bounds)
	std::map<int, luabridge::LuaRef> onMouseLeave;   // handle -> OnLeave(self) (cursor left bounds)
	std::set<int>                    hovered;        // handles the cursor is currently inside (edge detect)
	std::map<int, luabridge::LuaRef> onMouseDown;    // handle -> OnMouseDown(self, button)
	std::map<int, luabridge::LuaRef> onMouseUp;      // handle -> OnMouseUp(self, button)
	std::map<int, luabridge::LuaRef> onMouseWheel;   // handle -> OnMouseWheel(self, delta)
	std::map<int, luabridge::LuaRef> onDragStart;    // handle -> OnDragStart(self)
	std::map<int, luabridge::LuaRef> onDragStop;     // handle -> OnDragStop(self)
	std::map<int, luabridge::LuaRef> onReceiveDrag;  // handle -> OnReceiveDrag(self)
	std::map<int, luabridge::LuaRef> onShow;         // handle -> OnShow(self)
	std::map<int, luabridge::LuaRef> onHide;         // handle -> OnHide(self)
	std::map<int, luabridge::LuaRef> onValueChanged; // handle -> OnValueChanged(self, value)
	std::map<int, luabridge::LuaRef> onMinMaxChanged;// handle -> OnMinMaxChanged(self, min, max)
	std::map<int, luabridge::LuaRef> onSizeChanged;  // handle -> OnSizeChanged(self, w, h)
	std::set<int>                    shownState;     // last-seen shown flag (OnShow/OnHide edge detect)
	std::map<int, float>             lastValue;      // OnValueChanged edge cache
	std::map<int, std::pair<float,float>> lastMinMax;// OnMinMaxChanged edge cache
	std::map<int, std::pair<int,int>>     lastSize;  // OnSizeChanged edge cache
	std::map<int, luabridge::LuaRef> onEvent;        // handle -> OnEvent(self, event, arg)
	std::map<std::string, std::set<int>> eventSubs;  // eventName -> subscribed handles
	// RegisterUnitEvent: handle -> event -> allowed unit tokens. If a (handle,event) has a filter, fire()
	// only dispatches when the event's unit-token arg is in the set (WoW Frame:RegisterUnitEvent).
	std::map<int, std::map<std::string, std::set<std::string>>> unitFilters;
	struct TtSpec { int kind = 0; int key = 0; int anchor = 1; };   // kind 1=item 2=equip 3=stat; anchor 0..4
	std::map<int, TtSpec> tooltipSpec;               // handle -> spec; engine re-asserts the topmost hovered one

	std::vector<AddonInfo> addons;                  // loaded addons (M5)

	size_t memUsed = 0;
	size_t memCap  = 0;

	bool      instrArmed  = false;       // watchdog only bites around untrusted calls
	long long instrCount  = 0;
	long long instrBudget = 0;

	std::thread::id mainThreadId;
	bool reloadRequested = false;        // /reload sets this; onFrame performs it at a frame boundary
};

namespace
{
	constexpr size_t    kMemCap      = 128u * 1024u * 1024u; // 128 MB hard cap for the whole Lua VM
	constexpr int       kHookStep    = 1000;                 // count-hook granularity (VM instructions)
	constexpr long long kInstrBudget = 20'000'000;           // per host->Lua call (~a few ms) before kill

	// Lightweight Lua-facing handle for a widget; forwards to the LuaUI:: backend (no client types here).
	struct FrameHandle { int h = 0; };

	// TU-local singletons so every binding can be a *captureless* lambda (LuaBridge wants function pointers).
	LuaEngineImpl* g_impl   = nullptr;
	LuaEngine*     g_engine = nullptr;

	int pointToInt(const std::string& p)
	{
		if (p == "CENTER")      return LuaUI::Center;
		if (p == "TOP")         return LuaUI::Top;
		if (p == "BOTTOM")      return LuaUI::Bottom;
		if (p == "LEFT")        return LuaUI::Left;
		if (p == "RIGHT")       return LuaUI::Right;
		if (p == "TOPRIGHT")    return LuaUI::TopRight;
		if (p == "BOTTOMLEFT")  return LuaUI::BottomLeft;
		if (p == "BOTTOMRIGHT") return LuaUI::BottomRight;
		return LuaUI::TopLeft;
	}

	// Push a captured Lua function + args, then pcall under the instruction watchdog. No C++ exceptions:
	// errors come back through pcall's status and get logged. `what` labels the error line.
	template <class... Args>
	void callLua(const char* what, const luabridge::LuaRef& fn, Args&&... args)
	{
		if (!fn.isFunction())
			return;
		lua_State* L = g_impl->L;
		fn.push(L);
		(static_cast<void>(luabridge::Stack<std::decay_t<Args>>::push(L, std::forward<Args>(args))), ...);
		g_impl->instrArmed = true;
		g_impl->instrCount = 0;
		const int rc = lua_pcall(L, static_cast<int>(sizeof...(Args)), 0, 0);
		g_impl->instrArmed = false;
		if (rc != LUA_OK)
		{
			const char* e = lua_tostring(L, -1);
			g_engine->hostPrint(std::string(what) + ": " + (e ? e : "(error)"));
			lua_pop(L, 1);
		}
	}
}

// --- custom allocator: enforce a global memory cap (refusing => Lua raises "not enough memory") ---
static void* luaAlloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	auto* impl = static_cast<LuaEngineImpl*>(ud);

	if (nsize == 0)
	{
		if (ptr) impl->memUsed -= osize;
		std::free(ptr);
		return nullptr;
	}

	const size_t prev      = (ptr ? osize : 0);
	const size_t projected = impl->memUsed - prev + nsize;
	if (projected > impl->memCap)
		return nullptr;

	void* np = std::realloc(ptr, nsize);
	if (!np)
		return nullptr;

	impl->memUsed = projected;
	return np;
}

// --- instruction-count watchdog: kills runaway/infinite loops in untrusted code ---
static void instrHook(lua_State* L, lua_Debug*)
{
	void* ud = nullptr;
	lua_getallocf(L, &ud);                       // our allocator ud == the impl pointer
	auto* impl = static_cast<LuaEngineImpl*>(ud);
	if (!impl || !impl->instrArmed)
		return;

	impl->instrCount += kHookStep;
	if (impl->instrCount > impl->instrBudget)
		luaL_error(L, "%s", "exceeded the instruction budget - possible infinite loop");
}

LuaEngine::LuaEngine() : m_impl(std::make_unique<LuaEngineImpl>()) {}
LuaEngine::~LuaEngine() { shutdown(); }

LuaEngine* LuaEngine::instance()
{
	static LuaEngine inst;
	return &inst;
}

bool LuaEngine::isReady() const { return m_impl->L != nullptr; }

void LuaEngine::setOutputSink(std::function<void(const std::string&)> sink)
{
	m_impl->outputSink = std::move(sink);
}

void LuaEngine::init()
{
	if (m_impl->L)
		return;

	g_impl   = m_impl.get();
	g_engine = this;

	m_impl->memCap      = kMemCap;
	m_impl->instrBudget = kInstrBudget;
	m_impl->mainThreadId = std::this_thread::get_id();

	// Lua 5.5: lua_newstate takes a seed (was 2-arg in 5.4).
	m_impl->L = lua_newstate(luaAlloc, m_impl.get(), luaL_makeseed(nullptr));
	assert(m_impl->L != nullptr);

	lua_atpanic(m_impl->L, [](lua_State* L) -> int {
		const char* msg = lua_tostring(L, -1);
		blog(Logger::LOG_ERROR, "[lua] PANIC: %s", msg ? msg : "(unknown)");
		return 0;
	});

	lua_sethook(m_impl->L, instrHook, LUA_MASKCOUNT, kHookStep);

	buildSandbox();
	bindUI();
	blog(Logger::LOG_INFO, "[lua] engine ready (%s)", LUA_RELEASE);
}

void LuaEngine::shutdown()
{
	if (!m_impl->L)
		return;

	// Drop every LuaRef that references L BEFORE closing the state (RAII unref while L is still alive).
	clearFrames();
	m_impl->addons.clear();
	m_impl->sandbox.reset();
	m_impl->defaultUiEnv.reset();
	lua_close(m_impl->L);
	m_impl->L = nullptr;
	g_impl   = nullptr;
	g_engine = nullptr;
}

void LuaEngine::clearFrames()
{
	m_impl->onUpdate.clear();
	m_impl->onClick.clear();
	m_impl->onEnterPressed.clear();
	m_impl->onMouseEnter.clear();
	m_impl->onMouseLeave.clear();
	m_impl->hovered.clear();
	m_impl->onMouseDown.clear();
	m_impl->onMouseUp.clear();
	m_impl->onMouseWheel.clear();
	m_impl->onDragStart.clear();
	m_impl->onDragStop.clear();
	m_impl->onReceiveDrag.clear();
	m_impl->onShow.clear();
	m_impl->onHide.clear();
	m_impl->onValueChanged.clear();
	m_impl->onMinMaxChanged.clear();
	m_impl->onSizeChanged.clear();
	m_impl->shownState.clear();
	m_impl->lastValue.clear();
	m_impl->lastMinMax.clear();
	m_impl->lastSize.clear();
	m_impl->onEvent.clear();
	m_impl->eventSubs.clear();
	m_impl->unitFilters.clear();
	m_impl->tooltipSpec.clear();
}

// ---------------------------------------------------------------- addon loader (M5)

namespace
{
	bool readWholeFile(const std::filesystem::path& p, std::string& out)
	{
		std::ifstream f(p, std::ios::binary);
		if (!f) return false;
		std::ostringstream ss;
		ss << f.rdbuf();
		out = ss.str();
		return true;
	}

	std::string trimWs(const std::string& s)
	{
		const size_t a = s.find_first_not_of(" \t\r\n");
		if (a == std::string::npos) return "";
		const size_t b = s.find_last_not_of(" \t\r\n");
		return s.substr(a, b - a + 1);
	}

	// .toc = "## Key: Value" metadata lines + bare lines = ordered .lua files.
	bool parseToc(const std::filesystem::path& tocPath, AddonInfo& info)
	{
		std::string content;
		if (!readWholeFile(tocPath, content)) return false;

		std::istringstream in(content);
		std::string line;
		while (std::getline(in, line))
		{
			const std::string t = trimWs(line);
			if (t.empty()) continue;

			if (t.rfind("##", 0) == 0)
			{
				const std::string rest = trimWs(t.substr(2));
				const size_t colon = rest.find(':');
				if (colon == std::string::npos) continue;
				const std::string key = trimWs(rest.substr(0, colon));
				const std::string val = trimWs(rest.substr(colon + 1));
				if (key == "SavedVariables") info.savedVar = val;
				else if (key == "Enabled")   info.enabled  = !(val == "0" || val == "false");
				// Title / Version / Dependencies: ignored in v1
			}
			else if (t[0] != '#')
			{
				info.files.push_back(t);
			}
		}
		return true;
	}

	std::string luaQuote(const std::string& s)
	{
		std::string out = "\"";
		for (char c : s)
		{
			switch (c)
			{
				case '"':  out += "\\\""; break;
				case '\\': out += "\\\\"; break;
				case '\n': out += "\\n";  break;
				case '\r': out += "\\r";  break;
				case '\t': out += "\\t";  break;
				default:   out += c;      break;
			}
		}
		out += "\"";
		return out;
	}

	// Recursive Lua-literal writer over a value sitting at stack index `idx` (raw, lib-agnostic).
	void serializeStack(lua_State* L, int idx, std::ostream& os, int depth);

	void serializeTableAt(lua_State* L, int idx, std::ostream& os, int depth)
	{
		if (depth > 16) { os << "nil"; return; }   // cycle / over-deep guard
		idx = lua_absindex(L, idx);
		os << "{";
		bool first = true;
		lua_pushnil(L);
		while (lua_next(L, idx))   // key at -2, value at -1
		{
			const int vt = lua_type(L, -1);
			if (vt == LUA_TFUNCTION || vt == LUA_TUSERDATA || vt == LUA_TLIGHTUSERDATA || vt == LUA_TTHREAD)
			{
				lua_pop(L, 1);     // skip unserializable value, keep key for next()
				continue;
			}
			const int kt = lua_type(L, -2);
			std::string keyExpr;
			if (kt == LUA_TSTRING)
			{
				size_t len = 0;
				const char* ks = lua_tolstring(L, -2, &len);   // safe: key is a string here
				keyExpr = "[" + luaQuote(std::string(ks, len)) + "]=";
			}
			else if (kt == LUA_TNUMBER)
			{
				// copy the number out before formatting (don't lua_tostring the live key — it mutates it)
				const long long ki = static_cast<long long>(lua_tonumber(L, -2));
				keyExpr = "[" + std::to_string(ki) + "]=";
			}
			else
			{
				lua_pop(L, 1);
				continue;
			}

			if (!first) os << ",";
			first = false;
			os << keyExpr;
			serializeStack(L, -1, os, depth);
			lua_pop(L, 1);          // pop value, keep key for next()
		}
		os << "}";
	}

	void serializeStack(lua_State* L, int idx, std::ostream& os, int depth)
	{
		switch (lua_type(L, idx))
		{
			case LUA_TSTRING:
			{
				size_t len = 0;
				const char* s = lua_tolstring(L, idx, &len);
				os << luaQuote(std::string(s, len));
				break;
			}
			case LUA_TNUMBER:
			{
				const double d = lua_tonumber(L, idx);
				const long long i = static_cast<long long>(d);
				if (static_cast<double>(i) == d) os << i; else os << d;
				break;
			}
			case LUA_TBOOLEAN: os << (lua_toboolean(L, idx) ? "true" : "false"); break;
			case LUA_TTABLE:   serializeTableAt(L, idx, os, depth + 1); break;
			default:           os << "nil"; break;
		}
	}

	// Build a fresh per-addon env table whose reads fall back to the shared sandbox (metatable __index).
	luabridge::LuaRef makeChildEnv(lua_State* L, const luabridge::LuaRef& sandbox)
	{
		luabridge::LuaRef env = luabridge::newTable(L);
		luabridge::LuaRef mt  = luabridge::newTable(L);
		mt["__index"] = sandbox;
		env.push(L);
		mt.push(L);
		lua_setmetatable(L, -2);   // pops mt, sets it as env's metatable
		lua_pop(L, 1);             // pop env
		return env;
	}

	bool runChunkInEnv(LuaEngine* self, LuaEngineImpl* impl, const std::string& name,
	                   const std::string& src, const luabridge::LuaRef& env)
	{
		if (!src.empty() && static_cast<unsigned char>(src[0]) == 0x1B)
		{ self->hostPrint("addon " + name + ": bytecode rejected"); return false; }

		lua_State* L = impl->L;
		const std::string chunkName = "=" + name;
		if (luaL_loadbufferx(L, src.data(), src.size(), chunkName.c_str(), "t") != LUA_OK)
		{
			const char* e = lua_tostring(L, -1);
			self->hostPrint("addon " + name + " compile: " + (e ? e : "(error)"));
			lua_pop(L, 1);
			return false;
		}

		// Set the chunk's _ENV (upvalue 1 of a main chunk) to the sandboxed env.
		env.push(L);
		if (!lua_setupvalue(L, -2, 1))
			lua_pop(L, 1);   // chunk had no _ENV upvalue: drop the env we pushed

		impl->instrArmed = true;
		impl->instrCount = 0;
		const int rc = lua_pcall(L, 0, 0, 0);
		impl->instrArmed = false;
		if (rc != LUA_OK)
		{
			const char* e = lua_tostring(L, -1);
			self->hostPrint("addon " + name + ": " + (e ? e : "(error)"));
			lua_pop(L, 1);
			return false;
		}
		return true;
	}
}

// The built-in default UI: a shipped Lua "FrameXML" in ui/, loaded BEFORE user addons in one shared env so
// its files can share helpers. Absent ui/ => skip (the C++ HUD stays). Users override it (its frames are
// globals an addon can Hide()) or disable it (remove ui/).
void LuaEngine::loadDefaultUI()
{
	namespace fs = std::filesystem;
	std::error_code ec;
	const fs::path root = "ui";
	const fs::path toc  = root / "EmberUI.toc";
	if (!fs::is_directory(root, ec) || !fs::exists(toc))
		return;

	AddonInfo info;
	info.name = "ui";
	if (!parseToc(toc, info))
		return;

	m_impl->defaultUiEnv = makeChildEnv(m_impl->L, *m_impl->sandbox);   // shared, falls back to sandbox

	for (const auto& file : info.files)
	{
		std::string src;
		if (!readWholeFile(root / file, src))
		{
			hostPrint("default UI: missing file " + file);
			continue;
		}
		if (!runChunkInEnv(this, m_impl.get(), "ui/" + file, src, *m_impl->defaultUiEnv))
			break;   // a default-UI error stops loading the rest of it
	}
	blog(Logger::LOG_INFO, "[lua] default UI loaded");
}

void LuaEngine::loadAddons()
{
	if (!m_impl->L)
		init();

	loadDefaultUI();          // the built-in default UI loads first, so user addons can override it

	m_impl->addons.clear();   // fresh load (re-entering the world reloads; save happens on leave/reload)

	namespace fs = std::filesystem;
	std::error_code ec;
	const fs::path root = "addons";   // relative to the runtime cwd (the provisioned build/.../client dir)
	if (!fs::is_directory(root, ec))
		return;

	for (const auto& entry : fs::directory_iterator(root, ec))
	{
		if (!entry.is_directory())
			continue;
		const fs::path dir = entry.path();
		const std::string name = dir.filename().string();

		fs::path toc = dir / (name + ".toc");
		if (!fs::exists(toc))
			for (const auto& f : fs::directory_iterator(dir, ec))
				if (f.path().extension() == ".toc") { toc = f.path(); break; }
		if (!fs::exists(toc))
			continue;

		AddonInfo info;
		info.name = name;
		if (!parseToc(toc, info))
			continue;
		if (!info.enabled)
		{
			blog(Logger::LOG_INFO, "[lua] addon '%s' disabled", name.c_str());
			continue;
		}

		// Per-addon env: reads fall through to the shared sandbox; the addon's own globals stay local.
		luabridge::LuaRef env = makeChildEnv(m_impl->L, *m_impl->sandbox);
		{
			luabridge::LuaRef addonTbl = luabridge::newTable(m_impl->L);
			addonTbl["name"] = name;
			env["addon"] = addonTbl;
		}

		// SavedVariables: load the declared table (or start empty) into the env.
		if (!info.savedVar.empty())
		{
			luabridge::LuaRef sv = luabridge::newTable(m_impl->L);
			std::string svContent;
			if (readWholeFile(dir / "SavedVariables.lua", svContent) && !svContent.empty())
			{
				luabridge::LuaRef tmp = makeChildEnv(m_impl->L, *m_impl->sandbox);
				if (runChunkInEnv(this, m_impl.get(), name + ":SV", svContent, tmp))
				{
					luabridge::LuaRef loaded = tmp[info.savedVar];
					if (loaded.isTable())
						sv = loaded;
				}
			}
			env[info.savedVar] = sv;
		}

		bool ok = true;
		for (const auto& file : info.files)
		{
			std::string src;
			if (!readWholeFile(dir / file, src))
			{
				hostPrint("addon " + name + ": missing file " + file);
				ok = false;
				break;
			}
			if (!runChunkInEnv(this, m_impl.get(), name + "/" + file, src, env))
			{
				ok = false;   // broken addon: stop loading it, keep the others
				break;
			}
		}

		info.broken = !ok;
		info.env    = std::move(env);
		blog(Logger::LOG_INFO, "[lua] addon '%s' %s", name.c_str(), ok ? "loaded" : "DISABLED (error)");
		m_impl->addons.push_back(std::move(info));
	}

	// Default UI + all addons are loaded; settle z-order so level-tagged frames (loading screen, popups)
	// sit above default-level frames that addons appended.
	LuaUI::sortRootFrames();

	fire(LuaEvents::ADDON_LOADED, "");
	// PLAYER_LOGIN is fired later, from Game::processPacket_Server_SetController, when the player is
	// actually in the world (loading done) — not here at addon-load time.
}

void LuaEngine::saveAddons()
{
	if (!m_impl->L)
		return;

	namespace fs = std::filesystem;
	for (auto& a : m_impl->addons)
	{
		if (a.savedVar.empty() || a.broken || !a.env)
			continue;
		luabridge::LuaRef v = (*a.env)[a.savedVar];
		if (!v.isTable())
			continue;

		std::ostringstream os;
		os << a.savedVar << " = ";
		v.push(m_impl->L);
		serializeTableAt(m_impl->L, -1, os, 0);
		lua_pop(m_impl->L, 1);
		os << "\n";

		std::ofstream out(fs::path("addons") / a.name / "SavedVariables.lua", std::ios::binary | std::ios::trunc);
		if (out)
			out << os.str();
	}
}

void LuaEngine::reloadAddons()
{
	saveAddons();
	LuaUI::clearAllFrames();   // destroy Lua-created frames (manager side)
	clearFrames();             // drop OnUpdate/OnClick/OnEvent handlers (engine side)
	m_impl->addons.clear();    // release per-addon envs
	loadAddons();

	// loadAddons re-fires only ADDON_LOADED. If we reloaded while already in the world, the freshly
	// re-created HUD frames are still hidden (they reveal on WORLD_SHOWN/PLAYER_LOGIN, which won't fire
	// again on their own) — replay them so the HUD comes back instead of vanishing.
	if (LuaUI::isInWorld())
	{
		fire(LuaEvents::WORLD_SHOWN, "");
		fire(LuaEvents::PLAYER_LOGIN, "");
	}

	hostPrint("addons reloaded");
}

// ---------------------------------------------------------------- sandbox + bindings

// print(): join args via luaL_tolstring (respects __tostring) and route to the host console.
static int lua_print(lua_State* L)
{
	const int n = lua_gettop(L);
	std::string line;
	for (int i = 1; i <= n; ++i)
	{
		if (i > 1) line += '\t';
		size_t len = 0;
		const char* s = luaL_tolstring(L, i, &len);   // pushes the string repr
		line.append(s, len);
		lua_pop(L, 1);
	}
	if (g_engine) g_engine->hostPrint(line);
	return 0;
}

void LuaEngine::buildSandbox()
{
	lua_State* L = m_impl->L;

	// Open ONLY the safe standard libraries. io / os / debug / package are NEVER loaded into the VM.
	static const luaL_Reg kSafeLibs[] = {
		{ "_G",            luaopen_base },
		{ LUA_TABLIBNAME,  luaopen_table },
		{ LUA_STRLIBNAME,  luaopen_string },
		{ LUA_MATHLIBNAME, luaopen_math },
		{ LUA_COLIBNAME,   luaopen_coroutine },
		{ LUA_UTF8LIBNAME, luaopen_utf8 },
		{ nullptr, nullptr }
	};
	for (const luaL_Reg* lib = kSafeLibs; lib->func; ++lib)
	{
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);
	}

	// Strip the dangerous globals that luaopen_base installs (host mediates loading; addons can't reach disk).
	for (const char* n : { "dofile", "loadfile", "load", "loadstring", "collectgarbage", "require", "package" })
	{
		lua_pushnil(L);
		lua_setglobal(L, n);
	}

	// Build the whitelisted sandbox env. Addons get THIS as their _ENV; they never see the real _G.
	luabridge::LuaRef env = luabridge::newTable(L);
	for (const char* n : { "assert", "error", "ipairs", "pairs", "next", "select", "tonumber", "tostring",
	                       "type", "pcall", "xpcall", "rawequal", "rawget", "rawset", "rawlen",
	                       "setmetatable", "getmetatable", "_VERSION" })
		env[n] = luabridge::getGlobal(L, n);

	env["string"]    = luabridge::getGlobal(L, "string");
	env["table"]     = luabridge::getGlobal(L, "table");
	env["math"]      = luabridge::getGlobal(L, "math");
	env["coroutine"] = luabridge::getGlobal(L, "coroutine");
	env["utf8"]      = luabridge::getGlobal(L, "utf8");

	// Minimal, safe os.* only (no execute / getenv / remove / rename / exit / tmpname).
	luabridge::LuaRef osT = luabridge::newTable(L);
	osT["time"]  = static_cast<lua_CFunction>([](lua_State* L) -> int {
		lua_pushinteger(L, static_cast<lua_Integer>(std::time(nullptr))); return 1; });
	osT["clock"] = static_cast<lua_CFunction>([](lua_State* L) -> int {
		lua_pushnumber(L, static_cast<lua_Number>(std::clock()) / CLOCKS_PER_SEC); return 1; });
	env["os"] = osT;

	// Host-routed print() -> log + (optional) console sink.
	env["print"] = static_cast<lua_CFunction>(&lua_print);

	// In-sandbox _G is the addon's own env: stray globals only pollute the addon itself.
	env["_G"] = env;

	// Expose the event names to Lua as Events.UNIT_HEALTH etc. (same single source as the C++ fire sites).
	luabridge::LuaRef events = luabridge::newTable(L);
#define X(name) events[#name] = std::string(LuaEvents::name);
	LUA_EVENT_LIST(X)
#undef X
	env["Events"] = events;

	// Lock the shared string metatable so an addon can't repoint string.* for everyone.
	lua_pushliteral(L, "");
	if (lua_getmetatable(L, -1))
	{
		lua_pushliteral(L, "__metatable");
		lua_pushboolean(L, 0);
		lua_settable(L, -3);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	m_impl->sandbox = env;
}

namespace
{
	// SetScript router (kept free so the binding stays captureless).
	void setScript(int h, const std::string& which, const luabridge::LuaRef& fn)
	{
		auto* I = g_impl;
		if      (which == "OnUpdate")        I->onUpdate.insert_or_assign(h, fn);
		else if (which == "OnClick")         I->onClick.insert_or_assign(h, fn);
		else if (which == "OnEnter")         I->onMouseEnter.insert_or_assign(h, fn);   // cursor entered bounds
		else if (which == "OnLeave")         I->onMouseLeave.insert_or_assign(h, fn);   // cursor left bounds
		else if (which == "OnEnterPressed")  I->onEnterPressed.insert_or_assign(h, fn); // editbox Enter key
		else if (which == "OnMouseDown")   { I->onMouseDown.insert_or_assign(h, fn);  LuaUI::setMouseEnabled(h, true); }
		else if (which == "OnMouseUp")     { I->onMouseUp.insert_or_assign(h, fn);    LuaUI::setMouseEnabled(h, true); }
		else if (which == "OnMouseWheel")  { I->onMouseWheel.insert_or_assign(h, fn); LuaUI::setMouseEnabled(h, true); }
		else if (which == "OnDragStart")     I->onDragStart.insert_or_assign(h, fn);
		else if (which == "OnDragStop")      I->onDragStop.insert_or_assign(h, fn);
		else if (which == "OnReceiveDrag")   I->onReceiveDrag.insert_or_assign(h, fn);
		else if (which == "OnShow")          I->onShow.insert_or_assign(h, fn);
		else if (which == "OnHide")          I->onHide.insert_or_assign(h, fn);
		else if (which == "OnValueChanged")  I->onValueChanged.insert_or_assign(h, fn);
		else if (which == "OnMinMaxChanged") I->onMinMaxChanged.insert_or_assign(h, fn);
		else if (which == "OnSizeChanged")   I->onSizeChanged.insert_or_assign(h, fn);
		else if (which == "OnEvent")         I->onEvent.insert_or_assign(h, fn);
	}

	// WoW SetPoint(point [, relativeTo:Frame] [, relativePoint:string] [, x, y]) — variadic/poly, so raw.
	// self is at stack index 1 (Lua colon call); remaining args at 2.. with type-based dispatch.
	int frame_SetPoint(lua_State* L)
	{
		auto self = luabridge::Stack<FrameHandle>::get(L, 1);
		if (!self) return 0;
		const char* pointStr = lua_tostring(L, 2);
		const int pi = pointToInt(pointStr ? pointStr : "");
		int relH = 0, relPi = pi;   // default: anchor to owner/screen, relativePoint == point
		float x = 0.f, y = 0.f;
		int i = 3;
		const int top = lua_gettop(L);
		if (i <= top)
		{
			auto rel = luabridge::Stack<FrameHandle>::get(L, i);
			if (rel) { relH = rel->h; ++i; }
		}
		if (i <= top && lua_type(L, i) == LUA_TSTRING) { relPi = pointToInt(lua_tostring(L, i)); ++i; }
		if (i <= top && lua_isnumber(L, i))            { x = static_cast<float>(lua_tonumber(L, i)); ++i; }
		if (i <= top && lua_isnumber(L, i))            { y = static_cast<float>(lua_tonumber(L, i)); ++i; }
		LuaUI::setPoint(self->h, pi, relH, relPi, x, y);
		return 0;
	}
}

// Tooltip anchor name -> code (0=cursor, 1=right, 2=left, 3=top, 4=bottom). Default right.
static int anchorToInt(const std::string& a)
{
	if (a == "CURSOR") return 0;
	if (a == "LEFT")   return 2;
	if (a == "TOP")    return 3;
	if (a == "BOTTOM") return 4;
	return 1;   // RIGHT
}

void LuaEngine::bindUI()
{
	lua_State* L = m_impl->L;

	// FrameHandle methods live on a registry metatable; addons reach them via instances, not the sandbox env.
	luabridge::getGlobalNamespace(L)
		.beginClass<FrameHandle>("EmberFrame")
			.addFunction("SetPoint", &frame_SetPoint)
			.addFunction("SetAllPoints",   [](FrameHandle* self, std::optional<FrameHandle> rel) { LuaUI::setAllPoints(self->h, rel ? rel->h : 0); })
			.addFunction("ClearAllPoints", [](FrameHandle* self) { LuaUI::clearAllPoints(self->h); })
			.addFunction("SetSize",        [](FrameHandle* self, float w, float h) { LuaUI::setSize(self->h, w, h); })
			.addFunction("SetText",        [](FrameHandle* self, std::string t) { LuaUI::setText(self->h, t); })
			.addFunction("GetText",        [](FrameHandle* self) { return LuaUI::getText(self->h); })
			.addFunction("SetPassword",    [](FrameHandle* self, bool masked) { LuaUI::setEditBoxPassword(self->h, masked); })
			.addFunction("SetMaxLetters",  [](FrameHandle* self, int n) { LuaUI::setEditBoxMaxLen(self->h, n); })
			.addFunction("SetFocus",       [](FrameHandle* self) { LuaUI::focusEditBox(self->h, true); })
			.addFunction("ClearFocus",     [](FrameHandle* self) { LuaUI::focusEditBox(self->h, false); })
			.addFunction("HasFocus",       [](FrameHandle* self) { return LuaUI::editBoxFocused(self->h); })
			.addFunction("SetNumeric",     [](FrameHandle* self, bool v) { LuaUI::setEditBoxNumeric(self->h, v); })
			.addFunction("SetFontSize",    [](FrameHandle* self, int n) { LuaUI::setEditBoxFontSize(self->h, n); })
			.addFunction("SetTextColor",   [](FrameHandle* self, std::optional<int> r, std::optional<int> g, std::optional<int> b, std::optional<int> a) {
				LuaUI::setEditBoxColor(self->h, r.value_or(255), g.value_or(255), b.value_or(255), a.value_or(255)); })
			.addFunction("SetTexture",      [](FrameHandle* self, std::string t) { LuaUI::setTexture(self->h, t); })
			.addFunction("SetHoverTexture", [](FrameHandle* self, std::string t) { LuaUI::setHoverTexture(self->h, t); })
			.addFunction("SetHoverColor",   [](FrameHandle* self, int r, int g, int b, std::optional<int> a) { LuaUI::setHoverColor(self->h, r, g, b, a.value_or(255)); })
			.addFunction("SetFont",         [](FrameHandle* self, std::string f) { LuaUI::setFont(self->h, f); })
			.addFunction("SetWidth",        [](FrameHandle* self, int w) { LuaUI::setFontStringWidth(self->h, w); })
			.addFunction("SetVertexColor",  [](FrameHandle* self, std::optional<int> r, std::optional<int> g, std::optional<int> b, std::optional<int> a) {
				LuaUI::setVertexColor(self->h, r.value_or(255), g.value_or(255), b.value_or(255), a.value_or(255)); })
			.addFunction("SetStatusBarTexture", [](FrameHandle* self, std::string t) { LuaUI::setTexture(self->h, t); })
			.addFunction("SetMinMaxValues", [](FrameHandle* self, float mn, float mx) { LuaUI::setMinMax(self->h, mn, mx); })
			.addFunction("SetValue",        [](FrameHandle* self, float v) { LuaUI::setValue(self->h, v); })
			.addFunction("SetStatusBarColor", [](FrameHandle* self, std::optional<int> r, std::optional<int> g, std::optional<int> b, std::optional<int> a) {
				LuaUI::setBarColor(self->h, r.value_or(255), g.value_or(255), b.value_or(255), a.value_or(255)); })
			.addFunction("Show",           [](FrameHandle* self) { LuaUI::show(self->h, true); })
			.addFunction("Hide",           [](FrameHandle* self) { LuaUI::show(self->h, false); })
			.addFunction("IsValid",        [](FrameHandle* self) { return LuaUI::valid(self->h); })
			.addFunction("SetFrameLevel",  [](FrameHandle* self, int level) { LuaUI::setFrameLevel(self->h, level); })
			.addFunction("GetFrameLevel",  [](FrameHandle* self) { return LuaUI::getFrameLevel(self->h); })
			.addFunction("Raise",          [](FrameHandle* self) { LuaUI::raiseFrame(self->h); })
			.addFunction("Lower",          [](FrameHandle* self) { LuaUI::lowerFrame(self->h); })
			.addFunction("SetScript",      [](FrameHandle* self, std::string which, luabridge::LuaRef fn) { setScript(self->h, which, fn); })
			.addFunction("RegisterEvent",  [](FrameHandle* self, std::string event) { g_impl->eventSubs[event].insert(self->h); })
			// Fire only when the event's unit token matches unit1/unit2 (WoW RegisterUnitEvent).
			.addFunction("RegisterUnitEvent", [](FrameHandle* self, std::string event, std::string unit1, std::optional<std::string> unit2) {
				g_impl->eventSubs[event].insert(self->h);
				auto& set = g_impl->unitFilters[self->h][event];
				set.clear();
				set.insert(unit1);
				if (unit2) set.insert(*unit2); })
			.addFunction("UnregisterEvent",[](FrameHandle* self, std::string event) {
				auto it = g_impl->eventSubs.find(event);
				if (it != g_impl->eventSubs.end()) it->second.erase(self->h);
				auto uf = g_impl->unitFilters.find(self->h);
				if (uf != g_impl->unitFilters.end()) uf->second.erase(event); })
			.addFunction("UnregisterAllEvents", [](FrameHandle* self) {
				for (auto& kv : g_impl->eventSubs) kv.second.erase(self->h);
				g_impl->unitFilters.erase(self->h); })
			// Register a tooltip on the frame; the engine re-asserts it while the frame is hovered (no addon
			// OnUpdate poll needed). Pass 0/empty to clear.
			// Optional anchor: "RIGHT"(default)/"LEFT"/"TOP"/"BOTTOM"/"CURSOR" — where the tooltip sits vs the frame.
			.addFunction("SetTooltipItem",  [](FrameHandle* self, int bagSlot, std::optional<std::string> anchor) {
				g_impl->tooltipSpec[self->h] = { 1, bagSlot - 1, anchorToInt(anchor.value_or("RIGHT")) }; })
			.addFunction("SetTooltipEquip", [](FrameHandle* self, int equipSlot, std::optional<std::string> anchor) {
				g_impl->tooltipSpec[self->h] = { 2, equipSlot, anchorToInt(anchor.value_or("RIGHT")) }; })
			.addFunction("SetTooltipStat",  [](FrameHandle* self, int varId, std::optional<std::string> anchor) {
				if (varId > 0) g_impl->tooltipSpec[self->h] = { 3, varId, anchorToInt(anchor.value_or("LEFT")) };
				else           g_impl->tooltipSpec.erase(self->h); })
			.addFunction("SetTooltipLoot",  [](FrameHandle* self, int lootSlot, std::optional<std::string> anchor) {
				if (lootSlot > 0) g_impl->tooltipSpec[self->h] = { 4, lootSlot - 1, anchorToInt(anchor.value_or("RIGHT")) };
				else              g_impl->tooltipSpec.erase(self->h); })
			.addFunction("SetTooltipBank",  [](FrameHandle* self, int bankSlot, std::optional<std::string> anchor) {
				if (bankSlot > 0) g_impl->tooltipSpec[self->h] = { 5, bankSlot - 1, anchorToInt(anchor.value_or("RIGHT")) };
				else              g_impl->tooltipSpec.erase(self->h); })
			.addFunction("SetTooltipVendor", [](FrameHandle* self, int vendorIndex, std::optional<std::string> anchor) {
				if (vendorIndex > 0) g_impl->tooltipSpec[self->h] = { 6, vendorIndex - 1, anchorToInt(anchor.value_or("RIGHT")) };
				else                 g_impl->tooltipSpec.erase(self->h); })
			.addFunction("SetTooltipSpell", [](FrameHandle* self, int spellId, std::optional<std::string> anchor) {
				if (spellId > 0) g_impl->tooltipSpec[self->h] = { 7, spellId, anchorToInt(anchor.value_or("RIGHT")) };  // key = spellId
				else             g_impl->tooltipSpec.erase(self->h); })
			.addFunction("SetTooltipTrade", [](FrameHandle* self, int slot, bool isLocal, std::optional<std::string> anchor) {
				if (slot > 0) g_impl->tooltipSpec[self->h] = { 8, (slot - 1) + (isLocal ? 0 : 1000), anchorToInt(anchor.value_or("RIGHT")) };
				else          g_impl->tooltipSpec.erase(self->h); })
			.addFunction("SetTooltipAction", [](FrameHandle* self, int slot, std::optional<std::string> anchor) {   // key = action slot 1..36
				if (slot > 0) g_impl->tooltipSpec[self->h] = { 9, slot, anchorToInt(anchor.value_or("RIGHT")) };
				else          g_impl->tooltipSpec.erase(self->h); })
			.addFunction("SetTooltipChatLink", [](FrameHandle* self, int lineIdx, std::optional<std::string> anchor) {   // key = 1-based chat line
				if (lineIdx > 0) g_impl->tooltipSpec[self->h] = { 10, lineIdx - 1, anchorToInt(anchor.value_or("TOP")) };
				else             g_impl->tooltipSpec.erase(self->h); })
			.addFunction("SetTooltipItemEntry", [](FrameHandle* self, int itemEntry, std::optional<std::string> anchor) {   // key = raw item id
				if (itemEntry > 0) g_impl->tooltipSpec[self->h] = { 11, itemEntry, anchorToInt(anchor.value_or("RIGHT")) };
				else               g_impl->tooltipSpec.erase(self->h); })
			.addFunction("CreateTexture",    [](FrameHandle* self) { return FrameHandle{ LuaUI::createTexture(self->h) }; })
			.addFunction("CreateFontString", [](FrameHandle* self) { return FrameHandle{ LuaUI::createFontString(self->h) }; })
			.addFunction("IsMouseOver",    [](FrameHandle* self) { return LuaUI::isMouseOver(self->h); })
			.addFunction("GetLeft",        [](FrameHandle* self) { return LuaUI::frameLeft(self->h); })
			.addFunction("GetTop",         [](FrameHandle* self) { return LuaUI::frameTop(self->h); })
			.addFunction("GetWidth",       [](FrameHandle* self) { return LuaUI::frameWidth(self->h); })
			.addFunction("GetHeight",      [](FrameHandle* self) { return LuaUI::frameHeight(self->h); })
			.addFunction("GetParent",      [](FrameHandle* self) { return FrameHandle{ LuaUI::parentOf(self->h) }; })
			.addFunction("GetName",        [](FrameHandle* self) { return LuaUI::frameName(self->h); })
			.addFunction("GetObjectType",  [](FrameHandle* self) { return LuaUI::objectType(self->h); })
			.addFunction("IsShown",        [](FrameHandle* self) { return LuaUI::isShownSelf(self->h); })
			.addFunction("IsVisible",      [](FrameHandle* self) { return LuaUI::isVisible(self->h); })
			.addFunction("SetAlpha",       [](FrameHandle* self, float a) { LuaUI::setAlpha(self->h, a); })
			.addFunction("SetTexCoord",    [](FrameHandle* self, float l, float r, float t, float b) { LuaUI::setTexCoord(self->h, l, r, t, b); })
			.addFunction("SetCircular",    [](FrameHandle* self, int radius) { LuaUI::setTextureCircle(self->h, radius); })
			.addFunction("SetCooldown",    [](FrameHandle* self, int rem, int dur) { LuaUI::setCooldown(self->h, rem, dur); })
			.addFunction("GetCooldownRemaining", [](FrameHandle* self) { return LuaUI::cooldownRemaining(self->h); })
			.addFunction("SetScrollChild",          [](FrameHandle* self, std::optional<FrameHandle> child) { LuaUI::setScrollChild(self->h, child ? child->h : 0); })
			.addFunction("SetVerticalScroll",       [](FrameHandle* self, float v) { LuaUI::setVerticalScroll(self->h, v); })
			.addFunction("GetVerticalScroll",       [](FrameHandle* self) { return LuaUI::verticalScroll(self->h); })
			.addFunction("GetVerticalScrollRange",  [](FrameHandle* self) { return LuaUI::verticalScrollRange(self->h); })
			.addFunction("SetHorizontalScroll",     [](FrameHandle* self, float v) { LuaUI::setHorizontalScroll(self->h, v); })
			.addFunction("GetHorizontalScroll",     [](FrameHandle* self) { return LuaUI::horizontalScroll(self->h); })
			.addFunction("GetHorizontalScrollRange",[](FrameHandle* self) { return LuaUI::horizontalScrollRange(self->h); })
			.addFunction("SetScrollWheelStep",      [](FrameHandle* self, float px) { LuaUI::setScrollWheelStep(self->h, px); })
			.addFunction("EnableMouse",    [](FrameHandle* self, bool v) { LuaUI::setMouseEnabled(self->h, v); })
			.addFunction("SetMovable",     [](FrameHandle* self, bool v) { LuaUI::setMovable(self->h, v); if (v) LuaUI::setMouseEnabled(self->h, true); })
			.addFunction("RegisterForDrag",[](FrameHandle* self, std::optional<std::string> btn) {
				const std::string b = btn.value_or(std::string("LeftButton"));
				const int sfBtn = (b == "RightButton") ? 1 : (b == "MiddleButton") ? 2 : 0;
				LuaUI::setDragButton(self->h, sfBtn);
				LuaUI::setMovable(self->h, true);
				LuaUI::setMouseEnabled(self->h, true); })
		.endClass()

		// CreateFrame(frameType, name, parent) -> Frame. "Button" makes a clickable region; else a Frame.
		.addFunction("CreateFrame", [](std::optional<std::string> type, std::optional<std::string> name, std::optional<FrameHandle> parent) {
			const int p = parent ? parent->h : 0;
			const std::string t = type.value_or(std::string("Frame"));
			int h;
			if (t == "Button")         h = LuaUI::createButton(p);
			else if (t == "StatusBar") h = LuaUI::createStatusBar(p);
			else if (t == "Cooldown")  h = LuaUI::createCooldown(p);
			else if (t == "EditBox")   h = LuaUI::createEditBox(p);
			else if (t == "ScrollFrame") h = LuaUI::createScrollFrame(p);
			else                       h = LuaUI::createFrame(p);
			if (h && name && !name->empty())
				LuaUI::setName(h, *name);
			return FrameHandle{ h };
		})

		// Game-state getters.
		.addFunction("UnitHealth",    [](std::string token) { return LuaUI::unitHealth(token); })
		.addFunction("UnitHealthMax", [](std::string token) { return LuaUI::unitHealthMax(token); })
		.addFunction("UnitLevel",     [](std::string token) { return LuaUI::unitLevel(token); })
		.addFunction("UnitPower",     [](std::string token) { return LuaUI::unitPower(token); })
		.addFunction("UnitPowerMax",  [](std::string token) { return LuaUI::unitPowerMax(token); })
		.addFunction("UnitName",      [](std::string token) { return LuaUI::unitName(token); })
		.addFunction("UnitExists",    [](std::string token) { return LuaUI::unitExists(token); })
		.addFunction("GetXP",         []() { return LuaUI::playerXP(); })
		.addFunction("GetMaxXP",      []() { return LuaUI::playerMaxXP(); })
		.addFunction("GetMoney",      []() { return LuaUI::playerMoney(); })
		.addFunction("GetExperience", []() { return LuaUI::playerExperience(); })
		.addFunction("GetPlayerClassName", []() { return LuaUI::playerClassName(); })
		.addFunction("GetPlayerRankName",  []() { return LuaUI::playerRankName(); })
		.addFunction("GetStatRowCount",    [](int tab) { return LuaUI::statRowCount(tab); })
		.addFunction("GetStatRow",         [](int tab, int index) {   // -> label, value, r, g, b, tooltipVar
			std::string label, value; int rgb = 0, tv = 0;
			if (!LuaUI::statRow(tab, index, label, value, rgb, tv))
				return std::make_tuple(std::string(), std::string(), 0, 0, 0, 0);
			return std::make_tuple(label, value, (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, tv); })

			// Stat-point spending (drives the live force-hidden C++ Equipment; stats only).
			// statVar = a stat's variable id = the tooltipVar GetStatRow returns (0 rows are non-investable).
			.addFunction("IsSpendingPoints",      []() { return LuaUI::isSpendingPoints(); })
			.addFunction("BeginStatSpend",        []() { LuaUI::beginStatSpend(); })
			.addFunction("CancelStatSpend",       []() { LuaUI::cancelStatSpend(); })
			.addFunction("AddStatPoint",          [](int statVar) { return LuaUI::addStatPoint(statVar); })
			.addFunction("RemoveStatPoint",       [](int statVar) { return LuaUI::removeStatPoint(statVar); })
			.addFunction("CanAddStat",            [](int statVar) { return LuaUI::canAddStat(statVar); })
			.addFunction("CanMinusStat",          [](int statVar) { return LuaUI::canMinusStat(statVar); })
			.addFunction("GetPendingStatPoints",  [](int statVar) { return LuaUI::pendingStatPoints(statVar); })
			.addFunction("GetPendingLevelupCost", []() { return LuaUI::pendingLevelupCost(); })
			.addFunction("ConfirmStatSpend",      []() { LuaUI::confirmStatSpend(); })
			.addFunction("HasUnspentPoints",      []() { return LuaUI::hasUnspentPoints(); })

		// Bag / equipment / item data (read-only).
		.addFunction("GetContainerNumSlots", []() { return LuaUI::containerNumSlots(); })
		.addFunction("GetContainerItem", [](int slot) {   // slot 1-based -> (itemId, count, durability, enchant, soulbound)
			int id = 0, count = 0, dur = 0, ench = 0; bool sb = false;
			if (!LuaUI::containerItem(slot - 1, id, count, dur, ench, sb))
				return std::make_tuple(0, 0, 0, 0, false);
			return std::make_tuple(id, count, dur, ench, sb); })
		.addFunction("GetInventorySlotItem", [](int equipSlot) {   // EquipSlot 1..12 -> (itemId, durability, enchant)
			int id = 0, dur = 0, ench = 0;
			if (!LuaUI::equipItem(equipSlot, id, dur, ench))
				return std::make_tuple(0, 0, 0);
			return std::make_tuple(id, dur, ench); })
		.addFunction("GetItemInfo", [](int entry) {   // -> name, icon, quality, sellPrice, maxDurability
			std::string name, icon; int q = 0, sp = 0, md = 0;
			LuaUI::itemInfo(entry, name, icon, q, sp, md);
			return std::make_tuple(name, icon, q, sp, md); })
		.addFunction("GetItemQualityColor", [](int entry) {   // -> r, g, b (0..255)
			const int c = LuaUI::itemQualityColor(entry);
			return std::make_tuple((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF); })

		// Unit-frame parity getters.
		.addFunction("UnitNameColor", [](std::string token) {                          // -> r, g, b (0..255)
			const int c = LuaUI::unitNameColor(token);
			return std::make_tuple((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF); })
		.addFunction("UnitFlag",        [](std::string token, std::string name) { return LuaUI::unitFlag(token, name); })
		.addFunction("UnitIsDead",      [](std::string token) { return LuaUI::unitIsDead(token); })
		.addFunction("UnitIsPlayer",    [](std::string token) { return LuaUI::unitIsPlayer(token); })
		.addFunction("UnitIsPartyLeader", [](std::string token) { return LuaUI::unitIsPartyLeader(token); })
		.addFunction("UnitHasBrokenEquipment", [](std::string token) { return LuaUI::unitHasBrokenEquipment(token); })
		.addFunction("UnitPortraitTexture", [](std::string token) { return LuaUI::unitPortraitTexture(token); })
		.addFunction("UnitCastSpell",   [](std::string token) { return LuaUI::unitCastSpell(token); })
		.addFunction("UnitCastElapsed", [](std::string token) { return LuaUI::unitCastElapsedMs(token); })
		.addFunction("UnitCastTotal",   [](std::string token) { return LuaUI::unitCastTotalMs(token); })
		.addFunction("UnitAuraCount",   [](std::string token, bool harmful) { return LuaUI::unitAuraCount(token, harmful); })
		.addFunction("UnitAura",        [](std::string token, int index, bool harmful) {  // -> spellId, count, remainingMs, durationMs
			int spellId = 0, count = 0, remainingMs = 0, durationMs = 0;
			if (!LuaUI::unitAura(token, index - 1, harmful, spellId, count, remainingMs, durationMs))   // Lua is 1-based
				return std::make_tuple(0, 0, 0, 0);
			return std::make_tuple(spellId, count, remainingMs, durationMs); })
		.addFunction("PartyMemberExists", [](int idx) { return LuaUI::partyMemberExists(idx - 1); })   // Lua 1-based
		.addFunction("GetSpellTexture", [](int spellId) { return LuaUI::spellTexture(spellId); })
		.addFunction("GetSpellName",    [](int spellId) { return LuaUI::spellName(spellId); })
		.addFunction("GetSpellDescription", [](int spellId) { return LuaUI::spellDescription(spellId); })
		.addFunction("GetTextureSize",  [](std::string name) { return std::make_tuple(LuaUI::textureWidth(name), LuaUI::textureHeight(name)); })

		// Commands.
		.addFunction("TargetUnit",    [](std::string token) { LuaUI::targetUnit(token); })
		.addFunction("ClearTarget",   []() { LuaUI::clearTarget(); })

		// Item action commands (slots 1-based, like GetContainerItem).
		.addFunction("MoveContainerItem",    [](int from, int to) { LuaUI::moveContainerItem(from - 1, to - 1); })
		.addFunction("UseContainerItem",     [](int slot) { LuaUI::useContainerItem(slot - 1); })
		.addFunction("EquipContainerItem",   [](int slot) { LuaUI::equipContainerItem(slot - 1); })
		.addFunction("SellContainerItem",    [](int slot) { LuaUI::sellContainerItem(slot - 1); })
		.addFunction("DestroyContainerItem", [](int slot) { LuaUI::destroyContainerItem(slot - 1); })
		.addFunction("UnequipInventoryItem", [](int equipSlot, int invDest) { LuaUI::unequipItem(equipSlot, invDest - 1); })
		.addFunction("UseOrEquipContainerItem", [](int slot) { LuaUI::useOrEquipContainerItem(slot - 1); })
		.addFunction("MerchantRightClick",      [](int slot) { return LuaUI::containerItemMerchantAction(slot - 1); })

		// Loot window (1-based slot; reads/drives the live force-hidden LootWindow).
		.addFunction("GetLootSlotCount", []() { return LuaUI::lootSlotCount(); })
		.addFunction("GetLootSlot", [](int slot) {   // -> itemId, affix, count, isGold
			int id = 0, affix = 0, count = 0; bool gold = false;
			if (!LuaUI::lootSlot(slot - 1, id, affix, count, gold))
				return std::make_tuple(0, 0, 0, false);
			return std::make_tuple(id, affix, count, gold); })
		.addFunction("LootSlot",     [](int slot) { LuaUI::lootSlotTake(slot - 1); })
		.addFunction("LootAll",      []() { LuaUI::lootTakeAll(); })
		.addFunction("LinkLootSlot", [](int slot) { LuaUI::lootSlotLink(slot - 1); })
		.addFunction("CloseLoot",    []() { LuaUI::lootClose(); })
		.addFunction("IsShiftKeyDown", []() { return LuaUI::isShiftDown(); })

		// Bank (1-based slot).
		.addFunction("GetBankNumSlots", []() { return LuaUI::bankNumSlots(); })
		.addFunction("GetBankItem", [](int slot) {   // -> itemId, count, durability, enchant, soulbound
			int id = 0, count = 0, dur = 0, ench = 0; bool sb = false;
			if (!LuaUI::bankItem(slot - 1, id, count, dur, ench, sb))
				return std::make_tuple(0, 0, 0, 0, false);
			return std::make_tuple(id, count, dur, ench, sb); })
		.addFunction("WithdrawBankItem", [](int slot) { LuaUI::withdrawBankItem(slot - 1); })
		.addFunction("WithdrawBankItemTo", [](int bankSlot, int invSlot) { LuaUI::withdrawBankItemTo(bankSlot - 1, invSlot - 1); })
		.addFunction("MoveBankItem",     [](int from, int to) { LuaUI::moveBankItem(from - 1, to - 1); })
		.addFunction("DepositBagItem",   [](int bagSlot, int bankSlot) { LuaUI::depositBagItem(bagSlot - 1, bankSlot - 1); })
		.addFunction("SortBank",         []() { LuaUI::sortBank(); })
		.addFunction("CloseBank",        []() { LuaUI::bankClose(); })

		// Guild roster.
		.addFunction("GetGuildName",       []() { return LuaUI::guildName(); })
		.addFunction("GetGuildMotd",       []() { return LuaUI::guildMotd(); })
		.addFunction("GetNumGuildMembers", []() { return LuaUI::guildNumMembers(); })
		.addFunction("GetGuildLocalRank",  []() { return LuaUI::guildLocalRank(); })
		.addFunction("GetGuildMember", [](int index) {   // -> name, level, online, guid, className, r, g, b, rankName, rank
			std::string name, cls, rankName; int level = 0, guid = 0, color = 0, rank = 0; bool online = false;
			if (!LuaUI::guildMember(index - 1, name, level, online, guid, cls, color, rankName, rank))
				return std::make_tuple(std::string(), 0, false, 0, std::string(), 0, 0, 0, std::string(), 0);
			return std::make_tuple(name, level, online, guid, cls, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, rankName, rank); })
		.addFunction("SetGuildMotd",  [](std::string text) { LuaUI::setGuildMotd(text); })
		.addFunction("GuildPromote",  [](int guid) { LuaUI::guildPromote(guid); })
		.addFunction("GuildDemote",   [](int guid) { LuaUI::guildDemote(guid); })
		.addFunction("GuildKick",     [](int guid) { LuaUI::guildKick(guid); })
		.addFunction("InviteToParty", [](std::string name) { LuaUI::invitePlayerToParty(name); })
		.addFunction("RequestGuildRoster", []() { LuaUI::requestGuildRoster(); })
		.addFunction("WhisperPlayer", [](std::string name) { LuaUI::whisperPlayer(name); })

		// Quest log (1-based list index).
		.addFunction("GetNumQuests", []() { return LuaUI::questCount(); })
		.addFunction("GetQuestInfo", [](int index) {   // -> questId, title, done, tracked
			int id = 0; std::string title; bool done = false, tracked = false;
			if (!LuaUI::questInfo(index - 1, id, title, done, tracked))
				return std::make_tuple(0, std::string(), false, false);
			return std::make_tuple(id, title, done, tracked); })
		.addFunction("GetQuestObjectives",  [](int questId) { return LuaUI::questObjectives(questId); })
		.addFunction("GetQuestDescription", [](int questId) { return LuaUI::questDescription(questId); })
		.addFunction("IsQuestTracked",      [](int questId) { return LuaUI::questTracked(questId); })
		.addFunction("SetQuestTracked",     [](int questId, bool track) { LuaUI::setQuestTracked(questId, track); })
		.addFunction("AbandonQuest",        [](int questId) { LuaUI::abandonQuest(questId); })

		// Vendor (1-based item index; money = player money via GetMoney).
		.addFunction("GetVendorNumItems", []() { return LuaUI::vendorCount(); })
		.addFunction("GetVendorItem", [](int index) {   // -> itemId, affix, cost, supply
			int id = 0, affix = 0, cost = 0, supply = 0;
			if (!LuaUI::vendorItem(index - 1, id, affix, cost, supply))
				return std::make_tuple(0, 0, 0, 0);
			return std::make_tuple(id, affix, cost, supply); })
		.addFunction("BuyVendorItem", [](int index, int count) { LuaUI::buyVendorItem(index - 1, count); })
		.addFunction("RepairGear",    []() { LuaUI::repairGear(); })
		.addFunction("VendorBuyback", []() { LuaUI::vendorBuyback(); })

		// Abilities / spellbook (display-only; stage 0=Miscbook, 1=Spellbook; 1-based slot).
		.addFunction("GetNumSpellSlots", [](int stage) { return LuaUI::abilityCount(stage); })
		.addFunction("GetSpellSlot", [](int stage, int index) {   // -> spellId, level
			int spellId = 0, level = 0;
			if (!LuaUI::abilitySlot(stage, index - 1, spellId, level))
				return std::make_tuple(0, 0);
			return std::make_tuple(spellId, level); })

		// Action bars (global slot 1..36; the C++ bars stay the cast/cooldown/cache engine, Lua draws them).
		.addFunction("SetActionBarsLuaView", [](bool v) { LuaUI::setActionBarsLuaView(v); })
		.addFunction("GetActionInfo", [](int slot) {   // -> type ("spell"/"item"/nil), entry, texture
			int type = 0, entry = 0; std::string tex;
			if (!LuaUI::actionInfo(slot, type, entry, tex) || entry == 0)
				return std::make_tuple(std::string(), 0, std::string());
			return std::make_tuple(std::string(type == 1 ? "spell" : "item"), entry, tex); })
		.addFunction("GetActionCooldown", [](int slot) {   // -> remainingMs, durationMs (0,0 = ready)
			int rem = 0, dur = 0;
			if (!LuaUI::actionCooldown(slot, rem, dur))
				return std::make_tuple(0, 0);
			return std::make_tuple(rem, dur); })
		.addFunction("GetActionUsable", [](int slot) {   // -> usable, outOfRange, outOfMana
			const int f = LuaUI::actionState(slot);
			const bool has = (f & 1) != 0;
			return std::make_tuple(has && (f & 2) == 0, (f & 4) != 0, (f & 8) != 0); })
		.addFunction("GetActionCount",   [](int slot) { return LuaUI::actionCount(slot); })
		.addFunction("GetActionKeybind", [](int slot) { return LuaUI::actionKeybind(slot); })
		.addFunction("UseAction",        [](int slot) { LuaUI::useActionSlot(slot); })
		.addFunction("PlaceAction",      [](int slot, std::string type, int entry) {
			LuaUI::placeActionSlot(slot, type == "spell" ? 1 : 0, entry); })
		.addFunction("ClearAction",      [](int slot) { LuaUI::clearActionSlot(slot); })

		// Character select / create screens (1-based slot; gender 0=female 1=male per PlayerDefines).
		.addFunction("GetNumCharacters", []() { return LuaUI::characterCount(); })
		.addFunction("GetCharacterInfo", [](int idx) {   // -> name, classId, level, portrait, gender
			std::string name; int classId = 0, level = 0, portrait = 0, gender = 0;
			if (!LuaUI::characterAt(idx - 1, name, classId, level, portrait, gender))
				return std::make_tuple(std::string(), 0, 0, 0, 0);
			return std::make_tuple(name, classId, level, portrait, gender); })
		.addFunction("EnterWorldWithCharacter", [](int idx) { LuaUI::enterCharacterSlot(idx - 1); })
		.addFunction("DeleteCharacter",  [](int idx) { LuaUI::deleteCharacterSlot(idx - 1); })
		.addFunction("GotoCharCreate",   []() { LuaUI::gotoCharCreate(); })
		.addFunction("GotoCharSelect",   []() { LuaUI::gotoCharSelect(); })
		.addFunction("CreateCharacter",  [](std::string name, int classId, int gender, int portrait) {
			return LuaUI::createCharacter(name, classId, gender, portrait); })
		.addFunction("GetNumPortraits",  [](int gender) { return LuaUI::portraitCount(gender); })
		.addFunction("GetPortraitInfo",  [](int id, int gender) {   // -> texture, cropOffsetY
			std::string tex; int off = 0;
			LuaUI::portraitInfo(id, gender, tex, off);
			return std::make_tuple(tex, off); })
		.addFunction("GetClassInfo",     [](int classId) {   // -> name, r, g, b
			std::string name; int rgb = 0;
			LuaUI::classInfo(classId, name, rgb);
			return std::make_tuple(name, (rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255); })

		// Minimap + HUD chrome (frame art/labels/buttons in Lua; the GPU map composite stays C++).
		.addFunction("SetHudLuaView",      [](bool v) { LuaUI::setHudLuaView(v); })
		.addFunction("GetMinimapZone",     []() { return LuaUI::minimapZone(); })
		.addFunction("GetMinimapChannel",  []() { return LuaUI::minimapChannel(); })
		.addFunction("HasMailLoot",        []() { return LuaUI::mailLootAvailable(); })
		.addFunction("RecoverMailLoot",    []() { LuaUI::recoverMailLoot(); })
		.addFunction("GetNumChatChannels", []() { return LuaUI::chatChannelCount(); })
		.addFunction("GetChatChannelInfo", [](int idx) {   // -> members, capacity (1-based channel)
			return std::make_tuple(LuaUI::chatChannelMembers(idx - 1), LuaUI::chatChannelCapacity()); })
		.addFunction("ChangeChatChannel",  [](int idx) { LuaUI::changeChatChannel(idx - 1); })
		.addFunction("ToggleHudPanel",     [](std::string name) { LuaUI::toggleHudPanel(name); })

		// HUD leftovers: quest-tracker click-through + the spend-xp / waypoint world buttons.
		.addFunction("OpenQuestLog",        []() { LuaUI::openQuestLog(); })
		.addFunction("LaunchSpendExp",      []() { LuaUI::launchSpendExp(); })
		.addFunction("QueryWaypoints",      []() { LuaUI::queryWaypoints(); })
		.addFunction("IsStandingOnWaypoint",[]() { return LuaUI::standingOnWaypoint(); })

		// NPC gossip dialog (1-based option index; type "dialog"/"quest"/"complete"/"vendor").
		.addFunction("GetGossipNpcName", []() { return LuaUI::gossipNpcName(); })
		.addFunction("GetGossipText",    []() { return LuaUI::gossipText(); })
		.addFunction("GetNumGossipOptions", []() { return LuaUI::gossipOptionCount(); })
		.addFunction("GetGossipOption", [](int idx) {   // -> typeStr, entry, label
			int type = 0, entry = 0; std::string label;
			if (!LuaUI::gossipOption(idx - 1, type, entry, label))
				return std::make_tuple(std::string(), 0, std::string());
			static const char* kTypes[] = { "dialog", "quest", "complete", "vendor" };
			const char* t = (type >= 0 && type < 4) ? kTypes[type] : "dialog";
			return std::make_tuple(std::string(t), entry, label); })
		.addFunction("SelectGossipOption", [](int idx) { LuaUI::selectGossipOption(idx - 1); })
		.addFunction("CloseGossip",        []() { LuaUI::closeGossip(); })

		// Quest offer / complete dialogs. CompleteQuest takes the 1-based reward CHOICE SLOT (0/nil = none).
		.addFunction("GetQuestOfferInfo", []() {   // -> questId, title, desc, objectives
			int id = 0; std::string title, desc, obj;
			if (!LuaUI::questOfferInfo(id, title, desc, obj))
				return std::make_tuple(0, std::string(), std::string(), std::string());
			return std::make_tuple(id, title, desc, obj); })
		.addFunction("AcceptQuestOffer",  []() { LuaUI::acceptQuestOffer(); })
		.addFunction("DeclineQuestOffer", []() { LuaUI::declineQuestOffer(); })
		.addFunction("GetQuestCompleteInfo", []() {   // -> questId, title, desc
			int id = 0; std::string title, desc;
			if (!LuaUI::questCompleteInfo(id, title, desc))
				return std::make_tuple(0, std::string(), std::string());
			return std::make_tuple(id, title, desc); })
		.addFunction("QuestCompleteNeedsChoice", []() { return LuaUI::questCompleteNeedsChoice(); })
		.addFunction("CompleteQuest", [](std::optional<int> choiceSlot) { LuaUI::completeQuest(choiceSlot.value_or(0) - 1); })
		.addFunction("GetQuestRewardInfo", [](int questId) {   // -> xp, money(copper)
			int xp = 0, money = 0;
			LuaUI::questRewardInfo(questId, xp, money);
			return std::make_tuple(xp, money); })
		.addFunction("GetQuestRewardItem", [](int questId, bool isChoice, int slot) {   // -> itemId, count, usable
			int itemId = 0, count = 0; bool usable = true;
			if (!LuaUI::questRewardItem(questId, isChoice, slot - 1, itemId, count, usable))
				return std::make_tuple(0, 0, false);
			return std::make_tuple(itemId, count, usable); })

		// Game chat (1-based line index; channel = ChatDefines value; the C++ GameChat stays the engine).
		.addFunction("SetChatLuaView",   [](bool v) { LuaUI::setChatLuaView(v); })
		.addFunction("GetChatLineCount", []() { return LuaUI::chatLineCount(); })
		.addFunction("GetChatLine", [](int idx) {   // -> text, r, g, b, hasLink, channel
			std::string text; int rgba = 0, channel = 0; bool hasLink = false;
			if (!LuaUI::chatLine(idx - 1, text, rgba, hasLink, channel))
				return std::make_tuple(std::string(), 0, 0, 0, false, 0);
			return std::make_tuple(text, (rgba >> 24) & 255, (rgba >> 16) & 255, (rgba >> 8) & 255, hasLink, channel); })
		.addFunction("GetChatSender", [](int idx) { return LuaUI::chatLineSender(idx - 1); })
		.addFunction("SubmitChat",    [](std::string text) { LuaUI::chatSubmit(text); })
		.addFunction("ChatSwapChannel", [](std::string typed) { return LuaUI::chatTrySwapChannel(typed); })
		.addFunction("GetChatPrefix", []() {   // -> text, r, g, b
			std::string text; int rgba = 0;
			LuaUI::chatPrefix(text, rgba);
			return std::make_tuple(text, (rgba >> 24) & 255, (rgba >> 16) & 255, (rgba >> 8) & 255); })
		.addFunction("GetCombatLogCount", []() { return LuaUI::combatLogCount(); })
		.addFunction("GetCombatLogLine", [](int idx) {   // -> text, r, g, b, category (1 out 2 in 3 heal 4 misc)
			std::string text; int rgba = 0, cat = 0;
			if (!LuaUI::combatLogLine(idx - 1, text, rgba, cat))
				return std::make_tuple(std::string(), 0, 0, 0, 0);
			return std::make_tuple(text, (rgba >> 24) & 255, (rgba >> 16) & 255, (rgba >> 8) & 255, cat); })

		// Trade (isLocal = our side; 1-based slot; itemGuid identifies a local item for removal).
		.addFunction("GetTradePartnerName", []() { return LuaUI::tradePartnerName(); })
		.addFunction("GetTradeItem", [](bool isLocal, int slot) {   // -> itemId, count, itemGuid
			int id = 0, count = 0, guid = 0;
			if (!LuaUI::tradeItem(isLocal, slot - 1, id, count, guid))
				return std::make_tuple(0, 0, 0);
			return std::make_tuple(id, count, guid); })
		.addFunction("GetTradeGold",   [](bool isLocal) { return LuaUI::tradeGold(isLocal); })
		.addFunction("IsTradeReady",   [](bool isLocal) { return LuaUI::tradeReady(isLocal); })
		.addFunction("AddTradeItem",   [](int bagSlot) { LuaUI::tradeAddItem(bagSlot - 1); })
		.addFunction("RemoveTradeItem",[](int itemGuid) { LuaUI::tradeRemoveItem(itemGuid); })
		.addFunction("SetTradeGold",   [](int amount) { LuaUI::tradeSetGold(amount); })
		.addFunction("ConfirmTrade",   []() { LuaUI::tradeConfirm(); })
		.addFunction("CancelTrade",    []() { LuaUI::tradeCancel(); })

		.addFunction("IsContainerItemUsable",   [](int slot) { return !LuaUI::containerItemUnusable(slot - 1); })
		.addFunction("ContainerItemTargetsItem",[](int slot) { return LuaUI::containerItemTargetsItem(slot - 1); })
		.addFunction("UseContainerItemOnItem",  [](int src, int tgt) { LuaUI::useContainerItemOnItem(src - 1, tgt - 1); })
		.addFunction("IsMouseButtonDown",       [](std::string btn) {
			const int b = (btn == "RightButton") ? 1 : (btn == "MiddleButton") ? 2 : 0;
			return LuaUI::mouseButtonDown(b); })
		.addFunction("ShowConfirm",             [](std::string msg) { return LuaUI::showConfirm(msg); })
		.addFunction("PopConfirm",              []() {
			int code = 0; bool yes = false;
			if (!LuaUI::popConfirm(code, yes)) return std::make_tuple(0, false);
			return std::make_tuple(code, yes); })
		.addFunction("UnitContextMenu", [](std::string token, std::optional<std::string> extra) { LuaUI::unitContextMenu(token, extra.value_or(std::string())); })
		.addFunction("ShowUnitTooltip", [](std::string token) { LuaUI::showUnitTooltip(token); })
		.addFunction("ShowSpellTooltip", [](int spellId) { LuaUI::showSpellTooltip(spellId); })
		.addFunction("SaveUISetting", [](std::string key, int value) { LuaUI::saveUISetting(key, value); })
		.addFunction("GetUISetting",  [](std::string key, int def) { return LuaUI::getUISetting(key, def); })

		// Hide/show a C++ window that a Lua view replaces.
		.addFunction("SetGameFrameShown", [](std::string name, bool shown) { LuaUI::setGameFrameShown(name, shown); })
		.addFunction("IsGameFrameShown",  [](std::string name) { return LuaUI::gameFrameShown(name); })

		// Login screen command/getter.
		.addFunction("SubmitLogin",   [](std::string user, std::string pass, bool remember) { LuaUI::submitLogin(user, pass, remember); })
		.addFunction("GetSavedLogin", []() { return LuaUI::getSavedLogin(); })

		// Screen metrics for Lua layout.
		.addFunction("GetScreenWidth",  []() { return LuaUI::screenWidth(); })
		.addFunction("GetScreenHeight", []() { return LuaUI::screenHeight(); })
		.addFunction("GetCursorPosition", []() { return std::make_tuple(LuaUI::cursorX(), LuaUI::cursorY()); })

		// Debug: DebugBounds(true) outlines every Lua widget.
		.addFunction("DebugBounds", [](bool v) { LuaUI::setDebugBounds(v); })
		.endNamespace();

	// Copy the registered API globals into the sandbox env (addons never see _G; they get exactly these).
	static const char* kApiNames[] = {
		"CreateFrame", "UnitHealth", "UnitHealthMax", "UnitLevel", "UnitPower", "UnitPowerMax", "UnitName",
		"UnitExists", "GetXP", "GetMaxXP", "GetMoney", "GetExperience", "GetPlayerClassName",
		"GetPlayerRankName", "GetStatRowCount", "GetStatRow",
		"IsSpendingPoints", "BeginStatSpend", "CancelStatSpend", "AddStatPoint", "RemoveStatPoint",
		"CanAddStat", "CanMinusStat", "GetPendingStatPoints", "GetPendingLevelupCost", "ConfirmStatSpend",
		"HasUnspentPoints",
		"GetContainerNumSlots", "GetContainerItem",
		"GetInventorySlotItem", "GetItemInfo", "GetItemQualityColor",
		"UnitNameColor", "UnitFlag", "UnitIsDead", "UnitIsPlayer",
		"UnitIsPartyLeader", "UnitHasBrokenEquipment", "UnitPortraitTexture", "UnitCastSpell",
		"UnitCastElapsed", "UnitCastTotal", "UnitAuraCount", "UnitAura", "PartyMemberExists",
		"GetSpellTexture", "GetSpellName", "GetSpellDescription", "GetTextureSize", "TargetUnit", "ClearTarget",
		"MoveContainerItem", "UseContainerItem", "EquipContainerItem", "SellContainerItem",
		"DestroyContainerItem", "UnequipInventoryItem", "UseOrEquipContainerItem", "MerchantRightClick",
		"GetLootSlotCount", "GetLootSlot", "LootSlot", "LootAll", "LinkLootSlot", "CloseLoot", "IsShiftKeyDown",
		"GetBankNumSlots", "GetBankItem", "WithdrawBankItem", "WithdrawBankItemTo", "MoveBankItem", "DepositBagItem", "SortBank", "CloseBank",
		"GetGuildName", "GetGuildMotd", "GetNumGuildMembers", "GetGuildLocalRank", "GetGuildMember",
		"SetGuildMotd", "GuildPromote", "GuildDemote", "GuildKick", "InviteToParty", "RequestGuildRoster",
		"WhisperPlayer",
		"GetNumQuests", "GetQuestInfo", "GetQuestObjectives", "GetQuestDescription", "IsQuestTracked",
		"SetQuestTracked", "AbandonQuest",
		"GetVendorNumItems", "GetVendorItem", "BuyVendorItem", "RepairGear", "VendorBuyback",
		"GetNumSpellSlots", "GetSpellSlot",
		"SetActionBarsLuaView", "GetActionInfo", "GetActionCooldown", "GetActionUsable", "GetActionCount",
		"GetActionKeybind", "UseAction", "PlaceAction", "ClearAction",
		"SetChatLuaView", "GetChatLineCount", "GetChatLine", "GetChatSender", "SubmitChat", "ChatSwapChannel",
		"GetChatPrefix", "GetCombatLogCount", "GetCombatLogLine",
		"GetGossipNpcName", "GetGossipText", "GetNumGossipOptions", "GetGossipOption", "SelectGossipOption",
		"CloseGossip", "GetQuestOfferInfo", "AcceptQuestOffer", "DeclineQuestOffer", "GetQuestCompleteInfo",
		"QuestCompleteNeedsChoice", "CompleteQuest", "GetQuestRewardInfo", "GetQuestRewardItem",
		"OpenQuestLog", "LaunchSpendExp", "QueryWaypoints", "IsStandingOnWaypoint",
		"SetHudLuaView", "GetMinimapZone", "GetMinimapChannel", "HasMailLoot", "RecoverMailLoot",
		"GetNumChatChannels", "GetChatChannelInfo", "ChangeChatChannel", "ToggleHudPanel",
		"GetNumCharacters", "GetCharacterInfo", "EnterWorldWithCharacter", "DeleteCharacter",
		"GotoCharCreate", "GotoCharSelect", "CreateCharacter", "GetNumPortraits", "GetPortraitInfo",
		"GetClassInfo",
		"GetTradePartnerName", "GetTradeItem", "GetTradeGold", "IsTradeReady", "AddTradeItem", "RemoveTradeItem",
		"SetTradeGold", "ConfirmTrade", "CancelTrade",
		"IsContainerItemUsable", "ContainerItemTargetsItem", "UseContainerItemOnItem",
		"IsMouseButtonDown", "ShowConfirm", "PopConfirm", "UnitContextMenu",
		"ShowUnitTooltip", "ShowSpellTooltip", "SaveUISetting", "GetUISetting", "SetGameFrameShown",
		"IsGameFrameShown",
		"SubmitLogin", "GetSavedLogin", "GetScreenWidth", "GetScreenHeight", "GetCursorPosition", "DebugBounds",
	};
	luabridge::LuaRef& env = *m_impl->sandbox;
	for (const char* n : kApiNames)
		env[n] = luabridge::getGlobal(L, n);
}

// ---------------------------------------------------------------- event dispatch + per-frame pump

void LuaEngine::fire(const std::string& event, const std::string& arg)
{
	if (!m_impl->L)
		return;

	assert(std::this_thread::get_id() == m_impl->mainThreadId);

	auto it = m_impl->eventSubs.find(event);
	if (it == m_impl->eventSubs.end() || it->second.empty())
		return;

	// Snapshot subscribers (a handler may (un)register during dispatch).
	std::vector<int> handles(it->second.begin(), it->second.end());
	for (int h : handles)
	{
		auto hit = m_impl->onEvent.find(h);
		if (hit == m_impl->onEvent.end())
			continue;
		// RegisterUnitEvent filter: if this handle registered THIS event with a unit list, only fire when the
		// event's unit-token arg matches one of them.
		auto uf = m_impl->unitFilters.find(h);
		if (uf != m_impl->unitFilters.end())
		{
			auto ef = uf->second.find(event);
			if (ef != uf->second.end() && ef->second.find(arg) == ef->second.end())
				continue;
		}
		callLua("OnEvent error", hit->second, FrameHandle{ h }, event, arg);
	}
}

void LuaEngine::showTimedPopup(const std::string& text, float seconds)
{
	// Encode "<seconds>|<text>" into the single event arg; the Lua popup splits on the first '|'.
	char buf[32];
	snprintf(buf, sizeof(buf), "%.2f|", seconds);
	fire(LuaEvents::UI_POPUP, std::string(buf) + text);
}

void LuaEngine::clearTimedPopups()
{
	fire(LuaEvents::UI_POPUP_CLEAR, "");
}

void LuaEngine::requestReload()
{
	m_impl->reloadRequested = true;
}

void LuaEngine::onFrame(float dt)
{
	if (!m_impl->L)
		return;

	assert(std::this_thread::get_id() == m_impl->mainThreadId);

	// Perform a requested /reload at this safe frame boundary (never mid-handler).
	if (m_impl->reloadRequested)
	{
		m_impl->reloadRequested = false;
		reloadAddons();
	}

	// OnUpdate(self, dt) for every frame that registered one (snapshot so handlers may mutate the set).
	{
		std::vector<std::pair<int, luabridge::LuaRef>> updates(m_impl->onUpdate.begin(), m_impl->onUpdate.end());
		for (auto& [h, fn] : updates)
			callLua("OnUpdate error", fn, FrameHandle{ h }, dt);
	}

	// Drain clicked buttons and fire OnClick(self, "LeftButton").
	for (int h = LuaUI::popClickedHandle(); h != 0; h = LuaUI::popClickedHandle())
	{
		auto it = m_impl->onClick.find(h);
		if (it != m_impl->onClick.end())
			callLua("OnClick error", it->second, FrameHandle{ h }, std::string("LeftButton"));
	}

	// Drain Enter-submitted editboxes and fire OnEnterPressed(self).
	for (int h = LuaUI::popSubmittedHandle(); h != 0; h = LuaUI::popSubmittedHandle())
	{
		auto it = m_impl->onEnterPressed.find(h);
		if (it != m_impl->onEnterPressed.end())
			callLua("OnEnterPressed error", it->second, FrameHandle{ h });
	}

	// Drain clicked context-menu lines and fire CONTEXT_MENU_CLICK(line) so Lua can handle its own entries
	// (e.g. unit-frame Lock/Unlock). C++ social actions were already routed in unregisterCtxMenu.
	for (std::string line; LuaUI::popMenuResult(line); )
		fire(LuaEvents::CONTEXT_MENU_CLICK, line);

	// Engine-driven tooltips: the topmost hovered frame with a registered tooltip spec re-asserts it each
	// frame (the Application clears tooltips per frame), so addons set a tooltip ONCE (frame:SetTooltip*) and
	// never OnUpdate-poll for it.
	{
		int bestH = 0, bestLvl = -2147483647, kind = 0, key = 0, anchor = 1;
		for (auto& [h, spec] : m_impl->tooltipSpec)
		{
			if (!LuaUI::isMouseOver(h)) continue;
			const int lvl = LuaUI::getFrameLevel(h);
			if (lvl >= bestLvl) { bestLvl = lvl; bestH = h; kind = spec.kind; key = spec.key; anchor = spec.anchor; }
		}
		if      (kind == 1) LuaUI::showItemTooltip(key, bestH, anchor);
		else if (kind == 2) LuaUI::showEquipTooltip(key, bestH, anchor);
		else if (kind == 3) LuaUI::showStatTooltip(key, bestH, anchor);
		else if (kind == 4) LuaUI::showLootTooltip(key, bestH, anchor);
		else if (kind == 5) LuaUI::showBankTooltip(key, bestH, anchor);
		else if (kind == 6) LuaUI::showVendorTooltip(key, bestH, anchor);
		else if (kind == 7) LuaUI::showSpellTooltipAt(key, bestH, anchor);   // key = spellId
		else if (kind == 8) LuaUI::showTradeTooltip(key, bestH, anchor);     // key = slot + (isLocal?0:1000)
		else if (kind == 9) LuaUI::showActionTooltip(key, bestH, anchor);    // key = action slot 1..36
		else if (kind == 10) LuaUI::showChatLinkTooltip(key, bestH, anchor); // key = chat line index (0-based)
		else if (kind == 11) LuaUI::showItemEntryTooltip(key, bestH, anchor); // key = raw item entry (rewards)
	}

	// Hover edge-detection: fire OnEnter(self) when the cursor enters a frame's bounds and OnLeave(self)
	// when it leaves. The ENGINE tracks state (m_impl->hovered) so addons never poll IsMouseOver in
	// OnUpdate. Only frames with an OnEnter/OnLeave script are tested; handlers fire on transitions only.
	{
		std::set<int> watched;
		for (auto& [h, fn] : m_impl->onMouseEnter) watched.insert(h);
		for (auto& [h, fn] : m_impl->onMouseLeave) watched.insert(h);
		for (int h : watched)
		{
			const bool over = LuaUI::isMouseOver(h);
			const bool was  = m_impl->hovered.count(h) != 0;
			if (over == was)
				continue;
			if (over) m_impl->hovered.insert(h);
			else      m_impl->hovered.erase(h);

			auto& tbl = over ? m_impl->onMouseEnter : m_impl->onMouseLeave;
			auto it = tbl.find(h);
			if (it != tbl.end())
				callLua(over ? "OnEnter error" : "OnLeave error", it->second, FrameHandle{ h });
		}
	}

	// Drain mouse/drag events the manager's input() pass queued; fire the matching handlers.
	{
		auto buttonName = [](int b) -> std::string { return b == 1 ? "RightButton" : b == 2 ? "MiddleButton" : "LeftButton"; };
		for (LuaUI::WidgetMouseEvent e; LuaUI::popMouseEvent(e); )
		{
			std::map<int, luabridge::LuaRef>* tbl = nullptr;
			switch (e.kind)
			{
				case LuaUI::WE_MouseDown:   tbl = &m_impl->onMouseDown;   break;
				case LuaUI::WE_MouseUp:     tbl = &m_impl->onMouseUp;     break;
				case LuaUI::WE_MouseWheel:  tbl = &m_impl->onMouseWheel;  break;
				case LuaUI::WE_DragStart:   tbl = &m_impl->onDragStart;   break;
				case LuaUI::WE_DragStop:    tbl = &m_impl->onDragStop;    break;
				case LuaUI::WE_ReceiveDrag: tbl = &m_impl->onReceiveDrag; break;
			}
			if (!tbl) continue;
			auto it = tbl->find(e.handle);
			if (it == tbl->end()) continue;
			if (e.kind == LuaUI::WE_MouseDown || e.kind == LuaUI::WE_MouseUp)
				callLua("mouse/drag handler error", it->second, FrameHandle{ e.handle }, buttonName(e.button));
			else if (e.kind == LuaUI::WE_MouseWheel)
				callLua("mouse/drag handler error", it->second, FrameHandle{ e.handle }, e.delta);
			else
				callLua("mouse/drag handler error", it->second, FrameHandle{ e.handle });
		}
	}

	// OnShow/OnHide: edge-detect the frame's OWN shown flag (engine-tracked, like hover).
	{
		std::set<int> watched;
		for (auto& [h, fn] : m_impl->onShow) watched.insert(h);
		for (auto& [h, fn] : m_impl->onHide) watched.insert(h);
		for (int h : watched)
		{
			const bool shown = LuaUI::isShownSelf(h);
			const bool was   = m_impl->shownState.count(h) != 0;
			if (shown == was) continue;
			if (shown) m_impl->shownState.insert(h); else m_impl->shownState.erase(h);
			auto& tbl = shown ? m_impl->onShow : m_impl->onHide;
			auto it = tbl.find(h);
			if (it != tbl.end())
				callLua(shown ? "OnShow error" : "OnHide error", it->second, FrameHandle{ h });
		}
	}

	// OnValueChanged (StatusBar value), edge-detected against a cache. First sighting seeds, doesn't fire.
	for (auto& [h, fn] : m_impl->onValueChanged)
	{
		const float v = LuaUI::statusBarValue(h);
		const bool seen = m_impl->lastValue.count(h) != 0;
		const float prev = seen ? m_impl->lastValue[h] : 0.f;
		m_impl->lastValue[h] = v;
		if (!seen || prev == v) continue;
		callLua("OnValueChanged error", fn, FrameHandle{ h }, v);
	}

	// OnMinMaxChanged (StatusBar min/max).
	for (auto& [h, fn] : m_impl->onMinMaxChanged)
	{
		const std::pair<float,float> mm{ LuaUI::statusBarMin(h), LuaUI::statusBarMax(h) };
		const bool seen = m_impl->lastMinMax.count(h) != 0;
		const std::pair<float,float> prev = seen ? m_impl->lastMinMax[h] : std::pair<float,float>{};
		m_impl->lastMinMax[h] = mm;
		if (!seen || prev == mm) continue;
		callLua("OnMinMaxChanged error", fn, FrameHandle{ h }, mm.first, mm.second);
	}

	// OnSizeChanged (any frame's current size).
	for (auto& [h, fn] : m_impl->onSizeChanged)
	{
		const std::pair<int,int> sz{ LuaUI::frameWidth(h), LuaUI::frameHeight(h) };
		const bool seen = m_impl->lastSize.count(h) != 0;
		const std::pair<int,int> prev = seen ? m_impl->lastSize[h] : std::pair<int,int>{};
		m_impl->lastSize[h] = sz;
		if (!seen || prev == sz) continue;
		callLua("OnSizeChanged error", fn, FrameHandle{ h }, sz.first, sz.second);
	}
}

// ---------------------------------------------------------------- console / selftest

bool LuaEngine::runString(const std::string& chunkName, const std::string& source)
{
	if (!m_impl->L)
		init();

	assert(std::this_thread::get_id() == m_impl->mainThreadId);  // Lua is main-thread only

	if (!source.empty() && static_cast<unsigned char>(source[0]) == 0x1B /* LUA_SIGNATURE */)
	{
		hostPrint("rejected: precompiled bytecode is not allowed");
		return false;
	}

	return runChunkInEnv(this, m_impl.get(), chunkName, source, *m_impl->sandbox);
}

void LuaEngine::consoleExec(const std::string& code)
{
	runString("console", code);
}

void LuaEngine::hostPrint(const std::string& line)
{
	blog(Logger::LOG_INFO, "[lua] %s", line.c_str());
	if (m_impl->outputSink)
		m_impl->outputSink(line);
}

void LuaEngine::selfTest()
{
	if (!m_impl->L)
		init();

	static const char* kSandboxChecks =
		"assert(io == nil, 'io leaked')\n"
		"assert(os ~= nil and os.execute == nil, 'os.execute leaked')\n"
		"assert(require == nil, 'require leaked')\n"
		"assert(package == nil, 'package leaked')\n"
		"assert(load == nil, 'load leaked')\n"
		"assert(loadfile == nil, 'loadfile leaked')\n"
		"assert(dofile == nil, 'dofile leaked')\n"
		"assert(debug == nil, 'debug leaked')\n"
		"assert(collectgarbage == nil, 'collectgarbage leaked')\n"
		"assert(string and table and math and os.time, 'safe libs missing')\n"
		"print('sandbox check OK')\n";

	if (runString("selftest", kSandboxChecks))
		hostPrint("sandbox selftest PASSED");
	else
		hostPrint("sandbox selftest FAILED");

	// Watchdog: an infinite loop must be killed (runString returns false) without hanging the client.
	const bool killed = !runString("watchdog", "while true do end");
	hostPrint(killed ? "watchdog killed infinite loop: PASSED"
	                 : "watchdog FAILED (loop was not interrupted)");
}
