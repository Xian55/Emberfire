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
#include "../../Logger.h"   // header-only blog(); no std/byte usage

#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <map>
#include <set>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cassert>

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
	std::map<int, sol::protected_function> onEvent;      // handle -> OnEvent(self, event, arg)
	std::map<std::string, std::set<int>>   eventSubs;    // eventName -> subscribed handles

	size_t memUsed = 0;
	size_t memCap  = kMemCap;

	bool      instrArmed  = false;       // watchdog only bites around untrusted calls
	long long instrCount  = 0;
	long long instrBudget = kInstrBudget;

	std::thread::id mainThreadId;
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
	m_impl->sandbox  = sol::environment();
	m_impl->tostring = sol::protected_function();
	lua_close(m_impl->L);
	m_impl->L = nullptr;
}

void LuaEngine::clearFrames()
{
	m_impl->onUpdate.clear();
	m_impl->onClick.clear();
	m_impl->onEvent.clear();
	m_impl->eventSubs.clear();
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
		"SetTexture",       [](FrameHandle self, std::string t)    { LuaUI::setTexture(self.h, t); },
		"Show",             [](FrameHandle self)                   { LuaUI::show(self.h, true); },
		"Hide",             [](FrameHandle self)                   { LuaUI::show(self.h, false); },
		"IsValid",          [](FrameHandle self)                   { return LuaUI::valid(self.h); },
		"SetScript",        [this](FrameHandle self, std::string which, sol::protected_function fn) {
			if (which == "OnUpdate")     m_impl->onUpdate[self.h] = fn;
			else if (which == "OnClick") m_impl->onClick[self.h]  = fn;
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
		const int h = (t == "Button") ? LuaUI::createButton(p) : LuaUI::createFrame(p);
		return FrameHandle{ h };
	};

	// Game-state getters (M4).
	m_impl->sandbox["UnitHealth"]    = [](std::string token) { return LuaUI::unitHealth(token); };
	m_impl->sandbox["UnitHealthMax"] = [](std::string token) { return LuaUI::unitHealthMax(token); };
	m_impl->sandbox["UnitLevel"]     = [](std::string token) { return LuaUI::unitLevel(token); };
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

void LuaEngine::onFrame(float dt)
{
	if (!m_impl->L)
		return;

	assert(std::this_thread::get_id() == m_impl->mainThreadId);

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
