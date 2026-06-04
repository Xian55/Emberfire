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

	private:
		void input() final;
		void render() final;

		shared_ptr<Sprite> m_background;
		shared_ptr<Sprite> m_backgroundGui;

		void* m_chosenPrompt{ nullptr };
		clock_t m_loginAttemptAt = 0;
		bool m_autoLoginDone = false;   // [Debug] AutoLogin: auto-fill test/test + submit (dev iteration)
};

