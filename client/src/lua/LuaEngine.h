#pragma once

#include <string>
#include <memory>
#include <functional>

// Sandboxed Lua engine (M1). PUC Lua 5.4 + sol2, hidden behind a pimpl so sol.hpp / the Lua C headers
// never leak into the rest of the client (only src/lua/*.cpp pay the sol2 compile cost). Main-thread only.
// Singleton accessed via the `sLua` macro, mirroring sContentMgr / sConfig.
struct LuaEngineImpl;

class LuaEngine
{
	public:
		static LuaEngine* instance();

		// Create the VM + build the sandbox (idempotent; safe to call lazily on first use).
		void init();
		void shutdown();
		bool isReady() const;

		// Run untrusted Lua TEXT in the shared sandbox env: rejects precompiled bytecode, runs with the
		// instruction watchdog armed, isolates errors via pcall. Returns false on error (logged to console+log).
		bool runString(const std::string& chunkName, const std::string& source);

		// Console `lua <code>` entry point.
		void consoleExec(const std::string& code);

		// Per-frame pump (called from the main loop): runs OnUpdate(self, dt) handlers + drains OnClick.
		void onFrame(float dt);

		// Fire a named event to all frames that RegisterEvent'd it -> OnEvent(self, event, arg). sol-free
		// signature so game code (Game.cpp packet handlers) can call it without pulling in sol2.
		void fire(const std::string& event, const std::string& arg = "");

		// Drop all OnUpdate/OnClick/OnEvent handlers + event subscriptions. Called by the frame manager
		// on (re)create/destroy so stale handles from a previous World don't linger or collide.
		void clearFrames();

		// Addon layer (M5). loadAddons: enumerate addons/<Name>/, parse .toc, run each in a sandboxed
		// per-addon env (SavedVariables injected). saveAddons: persist every addon's SavedVariables table.
		// reloadAddons: save -> tear down frames+handlers -> reload from disk (hot reload, /reload).
		void loadAddons();
		void saveAddons();
		void reloadAddons();
		void requestReload();   // defer a reload to the next frame boundary (/reload command)

		// M1 sandbox proof: asserts io/os.execute/require/load/dofile/debug/package absent, safe libs present,
		// and that an infinite loop is killed by the watchdog without hanging the client.
		void selfTest();

		// Routed Lua print(): goes to the log + the output sink (the in-game ConsoleWindow), never files.
		void hostPrint(const std::string& line);

		// Host registers where print()/errors are echoed on screen (set by ConsoleWindow). sol-free boundary.
		void setOutputSink(std::function<void(const std::string&)> sink);

	private:
		LuaEngine();
		~LuaEngine();
		LuaEngine(const LuaEngine&) = delete;
		LuaEngine& operator=(const LuaEngine&) = delete;

		void buildSandbox();
		void bindUI();        // registers CreateFrame + the Frame handle methods (M2)
		void loadDefaultUI(); // loads the built-in ui/ default UI before addons

		std::unique_ptr<LuaEngineImpl> m_impl;
};

#define sLua LuaEngine::instance()
