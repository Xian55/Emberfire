// LuaEngine.cpp is the SINGLE sol2 boundary in the client.
//
// It is compiled in ISOLATION (see client/CMakeLists.txt): no precompiled header, and with
// _HAS_STD_BYTE=1 (the rest of the client builds with _HAS_STD_BYTE=0 because it uses a bare `byte`
// identifier under `using namespace std`; sol2 needs std::byte). Therefore this file must NOT include
// stdafx.h or any heavy client header (they pull in `using namespace std` + the bare `byte`). It talks
// to the rest of the client only through plain primitives and a print-sink callback.

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "LuaEngine.h"
#include "LuaUI.h"           // sol-free handle boundary to the widget adapters
#include "LuaEvents.h"       // single source of truth for fired event names
#include "../../Logger.h"   // header-only blog(); no std/byte usage

#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{
	constexpr size_t    kMemCap      = 128u * 1024u * 1024u; // 128 MB hard cap for the whole Lua VM
	constexpr int       kHookStep    = 1000;                 // count-hook granularity (VM instructions)
	constexpr long long kInstrBudget = 20'000'000;           // per host->Lua call (~a few ms) before kill

	// Lightweight Lua-facing handle for a widget; forwards to the LuaUI:: backend (no client types here).
	struct FrameHandle { int h = 0; };

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

	// A loaded addon: its sandbox env + metadata from the .toc.
	struct AddonInfo
	{
		std::string name;
		std::string savedVar;                 // ## SavedVariables: <name>  (empty if none)
		std::vector<std::string> files;       // ordered .lua files
		bool enabled = true;
		bool broken  = false;
		sol::environment env;                 // per-addon _ENV (falls back to the shared sandbox)
	};
}

// All sol2 / Lua state lives here so it never leaks into the header.
struct LuaEngineImpl
{
	lua_State*              L = nullptr;
	sol::environment        sandbox;     // the whitelisted _ENV every chunk runs under
	sol::protected_function tostring;    // captured base tostring, for print()

	std::function<void(const std::string&)> outputSink;  // routes print() to the console (set by host)

	std::map<int, sol::protected_function> onUpdate;     // handle -> OnUpdate(self, dt)
	std::map<int, sol::protected_function> onClick;      // handle -> OnClick(self, button)
	std::map<int, sol::protected_function> onEnter;      // handle -> OnEnter(self) (editbox Enter)
	std::map<int, sol::protected_function> onEvent;      // handle -> OnEvent(self, event, arg)
	std::map<std::string, std::set<int>>   eventSubs;    // eventName -> subscribed handles

	std::vector<AddonInfo> addons;                       // loaded addons (M5)
	sol::environment       defaultUiEnv;                 // shared env for the built-in Lua default UI (ui/)

	size_t memUsed = 0;
	size_t memCap  = kMemCap;

	bool      instrArmed  = false;       // watchdog only bites around untrusted calls
	long long instrCount  = 0;
	long long instrBudget = kInstrBudget;

	std::thread::id mainThreadId;
	bool reloadRequested = false;        // /reload sets this; onFrame performs it at a frame boundary
};

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
		luaL_error(L, "%s", "exceeded the instruction budget - possible infinite loop");  // lua_pushfstring lacks %lld
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

	m_impl->mainThreadId = std::this_thread::get_id();

	m_impl->L = lua_newstate(luaAlloc, m_impl.get());
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

	// Drop sol handles that reference L BEFORE closing the state.
	m_impl->sandbox      = sol::environment();
	m_impl->defaultUiEnv = sol::environment();
	m_impl->tostring     = sol::protected_function();
	lua_close(m_impl->L);
	m_impl->L = nullptr;
}

void LuaEngine::clearFrames()
{
	m_impl->onUpdate.clear();
	m_impl->onClick.clear();
	m_impl->onEnter.clear();
	m_impl->onEvent.clear();
	m_impl->eventSubs.clear();
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

	void serializeValue(const sol::object& v, std::ostream& os, int depth);

	void serializeTable(const sol::table& t, std::ostream& os, int depth)
	{
		if (depth > 16) { os << "nil"; return; }   // cycle / over-deep guard
		os << "{";
		bool first = true;
		for (const auto& kv : t)
		{
			const sol::object key = kv.first;
			const sol::object val = kv.second;
			const sol::type vt = val.get_type();
			if (vt == sol::type::function || vt == sol::type::userdata || vt == sol::type::lightuserdata || vt == sol::type::thread)
				continue;
			if (!first) os << ",";
			first = false;
			if (key.get_type() == sol::type::string)
				os << "[" << luaQuote(key.as<std::string>()) << "]=";
			else if (key.get_type() == sol::type::number)
				os << "[" << static_cast<long long>(key.as<double>()) << "]=";
			else
				continue;
			serializeValue(val, os, depth);
		}
		os << "}";
	}

	void serializeValue(const sol::object& v, std::ostream& os, int depth)
	{
		switch (v.get_type())
		{
			case sol::type::string:  os << luaQuote(v.as<std::string>()); break;
			case sol::type::number:
			{
				const double d = v.as<double>();
				const long long i = static_cast<long long>(d);
				if (static_cast<double>(i) == d) os << i; else os << d;
				break;
			}
			case sol::type::boolean: os << (v.as<bool>() ? "true" : "false"); break;
			case sol::type::table:   serializeTable(v.as<sol::table>(), os, depth + 1); break;
			default:                 os << "nil"; break;
		}
	}

	bool runChunkInEnv(LuaEngine* self, LuaEngineImpl* impl, const std::string& name, const std::string& src, sol::environment& env)
	{
		if (!src.empty() && static_cast<unsigned char>(src[0]) == 0x1B)
		{ self->hostPrint("addon " + name + ": bytecode rejected"); return false; }

		sol::state_view lua(impl->L);
		sol::load_result chunk = lua.load_buffer(src.data(), src.size(), ("=" + name).c_str(), sol::load_mode::text);
		if (!chunk.valid()) { sol::error e = chunk; self->hostPrint("addon " + name + " compile: " + e.what()); return false; }

		sol::protected_function fn = chunk;
		sol::set_environment(env, fn);

		impl->instrArmed = true;
		impl->instrCount = 0;
		sol::protected_function_result r = fn();
		impl->instrArmed = false;
		if (!r.valid()) { sol::error e = r; self->hostPrint("addon " + name + ": " + e.what()); return false; }
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

	sol::state_view lua(m_impl->L);
	m_impl->defaultUiEnv = sol::environment(lua, sol::create, m_impl->sandbox);   // shared, falls back to sandbox

	for (const auto& file : info.files)
	{
		std::string src;
		if (!readWholeFile(root / file, src))
		{
			hostPrint("default UI: missing file " + file);
			continue;
		}
		if (!runChunkInEnv(this, m_impl.get(), "ui/" + file, src, m_impl->defaultUiEnv))
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

	sol::state_view lua(m_impl->L);

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
		info.env = sol::environment(lua, sol::create, m_impl->sandbox);
		info.env["addon"] = lua.create_table_with("name", name);

		// SavedVariables: load the declared table (or start empty) into the env.
		if (!info.savedVar.empty())
		{
			sol::table sv = lua.create_table();
			std::string svContent;
			if (readWholeFile(dir / "SavedVariables.lua", svContent) && !svContent.empty())
			{
				sol::environment tmp(lua, sol::create, m_impl->sandbox);
				if (runChunkInEnv(this, m_impl.get(), name + ":SV", svContent, tmp))
				{
					sol::object loaded = tmp[info.savedVar];
					if (loaded.get_type() == sol::type::table)
						sv = loaded.as<sol::table>();
				}
			}
			info.env[info.savedVar] = sv;
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
			if (!runChunkInEnv(this, m_impl.get(), name + "/" + file, src, info.env))
			{
				ok = false;   // broken addon: stop loading it, keep the others
				break;
			}
		}

		info.broken = !ok;
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
		if (a.savedVar.empty() || a.broken)
			continue;
		sol::object v = a.env[a.savedVar];
		if (v.get_type() != sol::type::table)
			continue;

		std::ostringstream os;
		os << a.savedVar << " = ";
		serializeTable(v.as<sol::table>(), os, 0);
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
	hostPrint("addons reloaded");
}

void LuaEngine::buildSandbox()
{
	lua_State* L = m_impl->L;
	sol::state_view lua(L);

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

	sol::global_table G = lua.globals();

	// Strip the dangerous globals that luaopen_base installs (host mediates loading; addons can't reach disk).
	for (const char* n : { "dofile", "loadfile", "load", "loadstring", "collectgarbage", "require", "package" })
		G[n] = sol::lua_nil;

	m_impl->tostring = G["tostring"];

	// Build the whitelisted sandbox env. Addons get THIS as their _ENV; they never see the real _G.
	sol::environment env(lua, sol::create);
	for (const char* n : { "assert", "error", "ipairs", "pairs", "next", "select", "tonumber", "tostring",
	                       "type", "pcall", "xpcall", "rawequal", "rawget", "rawset", "rawlen",
	                       "setmetatable", "getmetatable", "_VERSION" })
		env[n] = G[n];

	env["string"]    = G["string"];
	env["table"]     = G["table"];
	env["math"]      = G["math"];
	env["coroutine"] = G["coroutine"];
	env["utf8"]      = G["utf8"];

	// Minimal, safe os.* only (no execute / getenv / remove / rename / exit / tmpname).
	sol::table osT = lua.create_table();
	osT["time"]  = []() { return static_cast<long long>(std::time(nullptr)); };
	osT["clock"] = []() { return static_cast<double>(std::clock()) / CLOCKS_PER_SEC; };
	env["os"] = osT;

	// Host-routed print() -> log + (optional) console sink.
	env["print"] = [this](sol::variadic_args va) {
		std::string line;
		for (std::size_t i = 0; i < va.size(); ++i)
		{
			if (i) line += '\t';
			sol::protected_function_result r = m_impl->tostring(sol::object(va[i]));
			line += (r.valid() ? r.get<std::string>() : std::string("?"));
		}
		hostPrint(line);
	};

	// In-sandbox _G is the addon's own env: stray globals only pollute the addon itself.
	env["_G"] = env;

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

void LuaEngine::bindUI()
{
	sol::state_view lua(m_impl->L);

	// FrameHandle methods live on a global metatable; addons reach them via instances, not the sandbox env.
	lua.new_usertype<FrameHandle>("__EmberFrame",
		sol::no_constructor,
		"SetPoint", [](FrameHandle self, std::string point, sol::variadic_args va) {
			const float x = (va.size() >= 1) ? va[0].as<float>() : 0.f;
			const float y = (va.size() >= 2) ? va[1].as<float>() : 0.f;
			const int pi = pointToInt(point);
			LuaUI::setPoint(self.h, pi, 0, pi, x, y);
		},
		"SetSize",          [](FrameHandle self, float w, float h) { LuaUI::setSize(self.h, w, h); },
		"SetText",          [](FrameHandle self, std::string t)    { LuaUI::setText(self.h, t); },
		"GetText",          [](FrameHandle self)                   { return LuaUI::getText(self.h); },
		"SetPassword",      [](FrameHandle self, bool masked)      { LuaUI::setEditBoxPassword(self.h, masked); },
		"SetMaxLetters",    [](FrameHandle self, int n)            { LuaUI::setEditBoxMaxLen(self.h, n); },
		"SetNumeric",       [](FrameHandle self, bool v)           { LuaUI::setEditBoxNumeric(self.h, v); },
		"SetFontSize",      [](FrameHandle self, int n)            { LuaUI::setEditBoxFontSize(self.h, n); },
		"SetTextColor",     [](FrameHandle self, sol::variadic_args va) {
			const int r = va.size() >= 1 ? va[0].as<int>() : 255;
			const int g = va.size() >= 2 ? va[1].as<int>() : 255;
			const int b = va.size() >= 3 ? va[2].as<int>() : 255;
			const int a = va.size() >= 4 ? va[3].as<int>() : 255;
			LuaUI::setEditBoxColor(self.h, r, g, b, a);
		},
		"SetTexture",       [](FrameHandle self, std::string t)    { LuaUI::setTexture(self.h, t); },
		"SetHoverTexture",  [](FrameHandle self, std::string t)    { LuaUI::setHoverTexture(self.h, t); },
		"SetFont",          [](FrameHandle self, std::string f)    { LuaUI::setFont(self.h, f); },
		"SetVertexColor",   [](FrameHandle self, sol::variadic_args va) {
			const int r = va.size() >= 1 ? va[0].as<int>() : 255;
			const int g = va.size() >= 2 ? va[1].as<int>() : 255;
			const int b = va.size() >= 3 ? va[2].as<int>() : 255;
			const int a = va.size() >= 4 ? va[3].as<int>() : 255;
			LuaUI::setVertexColor(self.h, r, g, b, a);
		},
		"SetStatusBarTexture", [](FrameHandle self, std::string t) { LuaUI::setTexture(self.h, t); },
		"SetMinMaxValues",  [](FrameHandle self, float mn, float mx) { LuaUI::setMinMax(self.h, mn, mx); },
		"SetValue",         [](FrameHandle self, float v)          { LuaUI::setValue(self.h, v); },
		"SetStatusBarColor",[](FrameHandle self, sol::variadic_args va) {
			const int r = va.size() >= 1 ? va[0].as<int>() : 255;
			const int g = va.size() >= 2 ? va[1].as<int>() : 255;
			const int b = va.size() >= 3 ? va[2].as<int>() : 255;
			const int a = va.size() >= 4 ? va[3].as<int>() : 255;
			LuaUI::setBarColor(self.h, r, g, b, a);
		},
		"Show",             [](FrameHandle self)                   { LuaUI::show(self.h, true); },
		"Hide",             [](FrameHandle self)                   { LuaUI::show(self.h, false); },
		"IsValid",          [](FrameHandle self)                   { return LuaUI::valid(self.h); },
		"SetFrameLevel",    [](FrameHandle self, int level)        { LuaUI::setFrameLevel(self.h, level); },
		"GetFrameLevel",    [](FrameHandle self)                   { return LuaUI::getFrameLevel(self.h); },
		"Raise",            [](FrameHandle self)                   { LuaUI::raiseFrame(self.h); },
		"Lower",            [](FrameHandle self)                   { LuaUI::lowerFrame(self.h); },
		"SetScript",        [this](FrameHandle self, std::string which, sol::protected_function fn) {
			if (which == "OnUpdate")     m_impl->onUpdate[self.h] = fn;
			else if (which == "OnClick") m_impl->onClick[self.h]  = fn;
			else if (which == "OnEnter") m_impl->onEnter[self.h]  = fn;
			else if (which == "OnEvent") m_impl->onEvent[self.h]  = fn;
		},
		"RegisterEvent",    [this](FrameHandle self, std::string event) { m_impl->eventSubs[event].insert(self.h); },
		"UnregisterEvent",  [this](FrameHandle self, std::string event) {
			auto it = m_impl->eventSubs.find(event);
			if (it != m_impl->eventSubs.end()) it->second.erase(self.h);
		},
		"UnregisterAllEvents", [this](FrameHandle self) { for (auto& kv : m_impl->eventSubs) kv.second.erase(self.h); },
		"CreateTexture",    [](FrameHandle self)                   { return FrameHandle{ LuaUI::createTexture(self.h) }; },
		"CreateFontString", [](FrameHandle self)                   { return FrameHandle{ LuaUI::createFontString(self.h) }; }
	);

	// CreateFrame(frameType, name, parent) -> Frame. "Button" makes a clickable region; else a Frame.
	m_impl->sandbox["CreateFrame"] = [](sol::optional<std::string> type, sol::optional<std::string>, sol::optional<FrameHandle> parent) {
		const int p = parent ? parent->h : 0;
		const std::string t = type.value_or(std::string("Frame"));
		int h;
		if (t == "Button")         h = LuaUI::createButton(p);
		else if (t == "StatusBar") h = LuaUI::createStatusBar(p);
		else if (t == "EditBox")   h = LuaUI::createEditBox(p);
		else                       h = LuaUI::createFrame(p);
		return FrameHandle{ h };
	};

	// Game-state getters.
	m_impl->sandbox["UnitHealth"]    = [](std::string token) { return LuaUI::unitHealth(token); };
	m_impl->sandbox["UnitHealthMax"] = [](std::string token) { return LuaUI::unitHealthMax(token); };
	m_impl->sandbox["UnitLevel"]     = [](std::string token) { return LuaUI::unitLevel(token); };
	m_impl->sandbox["UnitPower"]     = [](std::string token) { return LuaUI::unitPower(token); };
	m_impl->sandbox["UnitPowerMax"]  = [](std::string token) { return LuaUI::unitPowerMax(token); };
	m_impl->sandbox["UnitName"]      = [](std::string token) { return LuaUI::unitName(token); };
	m_impl->sandbox["UnitExists"]    = [](std::string token) { return LuaUI::unitExists(token); };
	m_impl->sandbox["GetXP"]         = []() { return LuaUI::playerXP(); };
	m_impl->sandbox["GetMaxXP"]      = []() { return LuaUI::playerMaxXP(); };

	// Commands.
	m_impl->sandbox["TargetUnit"]    = [](std::string token) { LuaUI::targetUnit(token); };
	m_impl->sandbox["ClearTarget"]   = []() { LuaUI::clearTarget(); };

	// Hide/show a C++ window that a Lua view replaces.
	m_impl->sandbox["SetGameFrameShown"] = [](std::string name, bool shown) { LuaUI::setGameFrameShown(name, shown); };

	// Login screen command/getter.
	m_impl->sandbox["SubmitLogin"]   = [](std::string user, std::string pass, bool remember) { LuaUI::submitLogin(user, pass, remember); };
	m_impl->sandbox["GetSavedLogin"] = []() { return LuaUI::getSavedLogin(); };

	// Screen metrics for Lua layout.
	m_impl->sandbox["GetScreenWidth"]  = []() { return LuaUI::screenWidth(); };
	m_impl->sandbox["GetScreenHeight"] = []() { return LuaUI::screenHeight(); };

	// Debug: DebugBounds(true) outlines every Lua widget.
	m_impl->sandbox["DebugBounds"] = [](bool v) { LuaUI::setDebugBounds(v); };

	// Expose the event names to Lua as Events.UNIT_HEALTH etc. (same single source as the C++ fire sites).
	sol::table events = lua.create_table();
#define X(name) events[#name] = std::string(LuaEvents::name);
	LUA_EVENT_LIST(X)
#undef X
	m_impl->sandbox["Events"] = events;
}

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
		if (hit == m_impl->onEvent.end() || !hit->second.valid())
			continue;
		m_impl->instrArmed = true;
		m_impl->instrCount = 0;
		sol::protected_function_result r = hit->second(FrameHandle{ h }, event, arg);
		m_impl->instrArmed = false;
		if (!r.valid()) { sol::error e = r; hostPrint(std::string("OnEvent error: ") + e.what()); }
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
	std::vector<std::pair<int, sol::protected_function>> updates(m_impl->onUpdate.begin(), m_impl->onUpdate.end());
	for (auto& [h, fn] : updates)
	{
		if (!fn.valid())
			continue;
		m_impl->instrArmed = true;
		m_impl->instrCount = 0;
		sol::protected_function_result r = fn(FrameHandle{ h }, dt);
		m_impl->instrArmed = false;
		if (!r.valid()) { sol::error e = r; hostPrint(std::string("OnUpdate error: ") + e.what()); }
	}

	// Drain clicked buttons and fire OnClick(self, "LeftButton").
	for (int h = LuaUI::popClickedHandle(); h != 0; h = LuaUI::popClickedHandle())
	{
		auto it = m_impl->onClick.find(h);
		if (it == m_impl->onClick.end() || !it->second.valid())
			continue;
		m_impl->instrArmed = true;
		m_impl->instrCount = 0;
		sol::protected_function_result r = it->second(FrameHandle{ h }, std::string("LeftButton"));
		m_impl->instrArmed = false;
		if (!r.valid()) { sol::error e = r; hostPrint(std::string("OnClick error: ") + e.what()); }
	}

	// Drain Enter-submitted editboxes and fire OnEnter(self).
	for (int h = LuaUI::popSubmittedHandle(); h != 0; h = LuaUI::popSubmittedHandle())
	{
		auto it = m_impl->onEnter.find(h);
		if (it == m_impl->onEnter.end() || !it->second.valid())
			continue;
		m_impl->instrArmed = true;
		m_impl->instrCount = 0;
		sol::protected_function_result r = it->second(FrameHandle{ h });
		m_impl->instrArmed = false;
		if (!r.valid()) { sol::error e = r; hostPrint(std::string("OnEnter error: ") + e.what()); }
	}
}

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

	sol::state_view lua(m_impl->L);
	sol::load_result chunk = lua.load_buffer(source.data(), source.size(), ("=" + chunkName).c_str(), sol::load_mode::text);
	if (!chunk.valid())
	{
		sol::error e = chunk;
		hostPrint(std::string("compile error: ") + e.what());
		return false;
	}

	sol::protected_function fn = chunk;
	sol::set_environment(m_impl->sandbox, fn);

	m_impl->instrArmed = true;
	m_impl->instrCount = 0;
	sol::protected_function_result r = fn();
	m_impl->instrArmed = false;

	if (!r.valid())
	{
		sol::error e = r;
		hostPrint(std::string("error: ") + e.what());
		return false;
	}
	return true;
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
