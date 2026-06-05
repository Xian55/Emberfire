#pragma once

#include "RenderObjectHolder.h"

class Sprite;

class Login : public RenderObjectHolder
{
	public:
		enum Interface
		{
			LoginButton = 1,
			PhonyButtonUser,
			PhonyButtonPass,
			UserPrompt,
			PassPrompt,
			RememberTickbox
		};

	public:
		Login(RenderObjectHolder& owner, const int id);
		virtual ~Login();

		// Authenticate + connect + send the auth packet. Synchronous (HTTP auth + connect block, as the
		// original render() path did). Returns false + a human error on auth/connect failure; true when the
		// auth packet was sent (the server's Validate reply then drives the stage transition). Exposed so the
		// Lua login screen can drive login while the C++ Login is force-hidden.
		bool performLogin(const string& user, const string& pass, bool remember, string& errorOut);

	private:
		void input() final;
		void render() final;

		shared_ptr<Sprite> m_background;
		shared_ptr<Sprite> m_backgroundGui;

		void* m_chosenPrompt{ nullptr };
		clock_t m_loginAttemptAt = 0;
		bool m_autoLoginDone = false;   // [Debug] AutoLogin: auto-fill test/test + submit (dev iteration)
};

