#include "stdafx.h"
#include "Login.h"
#include "Application.h"
#include "Sprite.h"
#include "ContentMgr.h"
#include "Button.h"
#include "Connector.h"
#include "ConfirmMessageBox.h"
#include "PromptBox.h"
#include "TickBox.h"

#include "..\Shared\Config.h"
#include "..\Shared\AccountDefines.h"
#include "..\StringHelpers.h"
#include "..\Shared\Md5.h"
#include "..\Shared\GamePacketClient.h"
#include "..\Shared\DmystVersion.h"

#include <assert.h>

Login::Login(RenderObjectHolder& owner, const int id) :
	RenderObjectHolder(&owner, id)
{
	ASSERT(m_background = sContentMgr->spawnSprite("main_bg.jpg"));
	ASSERT(m_backgroundGui = sContentMgr->spawnSprite("main_login.png"));

	setMultiInput(true);
	sContentMgr->loopMusic(MusicId::AionCreation, false);	
	string defaultLogin = sConfig->getString("UI", "DefaultLogin", "");

	if (defaultLogin.size() <= 1)
		sApplication->spawnPopup("No account configured. Add one in the Emberfire auth server.", ConfirmMessageBox::ConfirmBox_Ok, 0);

	addRenderObject(attachObj(make_shared<TickBox>(*this, Interface::RememberTickbox, "tick_box_empty", "tick_box_full", sf::Vector2i{}, true), sf::Vector2i(260, 580)));

	// User prompt
	auto promptBox = make_shared<PromptBox>(*this, Interface::UserPrompt, FontId::Palatino, nullptr, sf::Vector2i{}, 300, sf::Color(220, 194, 171, 255), false);
	promptBox->setContent(defaultLogin);
	promptBox->setPromptCharacterSize(20);
	promptBox->setPromptMaxStrLen(AccountDefines::MaxUserLength);
	addRenderObject(attachObj(promptBox, sf::Vector2i(180, 400)));

	// Password prompt
	promptBox = make_shared<PromptBox>(*this, Interface::PassPrompt, FontId::Palatino, nullptr, sf::Vector2i{}, 400, sf::Color(157, 137, 119, 255), false);
	promptBox->setSafetyRender(true);
	promptBox->setPromptCharacterSize(24);
	promptBox->setPromptMaxStrLen(AccountDefines::MaxPassLength);
	addRenderObject(attachObj(promptBox, sf::Vector2i(180, 480)));
		
	// Only one prompt at a time, but also, if we have a default username remembered then go straight to entering your password
	m_chosenPrompt = defaultLogin.empty() ? getRenderObject(Interface::UserPrompt).get() : getRenderObject(Interface::PassPrompt).get();
		
	// Invisible buttons go on top
	addRenderObject(attachObj(make_shared<Button>(*this, "login_phony", Interface::PhonyButtonUser), sf::Vector2i(148, 390)));
	addRenderObject(attachObj(make_shared<Button>(*this, "login_phony", Interface::PhonyButtonPass), sf::Vector2i(148, 470)));
	addRenderObject(attachObj(make_shared<Button>(*this, "login", Interface::LoginButton, sf::Vector2i{}, SfKeyEvent(sf::Keyboard::Return)), sf::Vector2i(203, 649)));
}

Login::~Login()
{

}

void Login::input()
{
	// If we're connected to the server then don't accept any more input. 
	// Instead, wait for the server to push us to the next menu.
	if (sConnector->connected())
		return;
	
	sApplication->setCurrentPrompt(m_chosenPrompt);

	// Tab to swap between prompts
	if (sApplication->popKeyUp(sf::Keyboard::Tab))
	{
		const auto currentPromt = sApplication->getCurrentPromptInput();

		if (currentPromt == reinterpret_cast<decltype(currentPromt)>(getRenderObject(Interface::UserPrompt).get()))
			m_chosenPrompt = getRenderObject(Interface::PassPrompt).get();
		else
			m_chosenPrompt = getRenderObject(Interface::UserPrompt).get();
	}

	__super::input();

	if (auto confirmBox = sApplication->popConfirmBox({ ConfirmMessageBox::ConfirmCode_RetryLogin }))
	{
		switch (confirmBox->getCode())
		{
			case ConfirmMessageBox::ConfirmCode_RetryLogin:
			{
				if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
				{
					sApplication->spawnTimedPopup("Connecting...", 3.0f);

					// Intentionally have an extra wait here to ease up on spamming auth
					m_loginAttemptAt = ::clock() + 2000;
				}

				break;
			}
		}
	}

	switch (popFirstButtonId())
	{
		case Interface::PhonyButtonUser:
		{
			m_chosenPrompt = getRenderObject(Interface::UserPrompt).get();
			break;
		}
		case Interface::PhonyButtonPass:
		{
			m_chosenPrompt = getRenderObject(Interface::PassPrompt).get();
			break;
		}
		case Interface::LoginButton:
		{
			sApplication->spawnTimedPopup("Connecting...", 3.0f);
			m_loginAttemptAt = ::clock() + 100;
			break;
		}
	}
}

void Login::render()
{
	// Dev auto-login: [Debug] AutoLogin=1 fills test/test and submits once.
	if (!m_autoLoginDone && sConfig->getInt("Debug", "AutoLogin", 0))
	{
		m_autoLoginDone = true;
		const std::string acct = sConfig->getString("Debug", "AutoLoginUser", "test");
		const std::string pass = sConfig->getString("Debug", "AutoLoginPass", "test");
		if (auto u = dynamic_pointer_cast<PromptBox>(getRenderObject(Interface::UserPrompt))) u->setContent(acct);
		if (auto p = dynamic_pointer_cast<PromptBox>(getRenderObject(Interface::PassPrompt))) p->setContent(pass);
		m_loginAttemptAt = ::clock() + 500;
	}

	if (m_loginAttemptAt && ::clock() > m_loginAttemptAt)
	{
		string userStr = dynamic_pointer_cast<PromptBox>(getRenderObject(Interface::UserPrompt))->getContent();
		string passwordStr = dynamic_pointer_cast<PromptBox>(getRenderObject(Interface::PassPrompt))->getContent();
		string error;
		string token;

		// Auth host/port/scheme from config ([Auth] Host/Port/Secure); defaults to local node auth 127.0.0.1:80 HTTP.
		std::wstring authHost = Util::toUtf16(sConfig->getString("Auth", "Host", "127.0.0.1"));
		if (!sConnector->attemptAuth(authHost.c_str(), L"/auth/auth.php", userStr, passwordStr, token, error))
		{
			sApplication->spawnTimedPopup("Authentication failed: " + error, 3.0f);
		}

		else
		{
			// We now have a temporary token, try to connect to the game server with it if that passed
			// Grace period for the server
			if (const int graceMs = sConfig->getInt("Debug", "ServerGraceMs", 0)) Sleep(static_cast<DWORD>(graceMs)); // was hard Sleep(2000); default 0 = no artificial login wait (bump if a local token race appears)

			if (!sConnector->connect())
			{
				sApplication->spawnPopup("Failed to reach game server", ConfirmMessageBox::ConfirmBox_Ok, 0);
			}
			else
			{
				GP_Client_Authenticate packet;
				packet.m_userToken = token;
				packet.m_build = DYMST_VERSION;
				packet.m_fingerprint = sApplication->getMachineFingerprint();

				sConnector->sendPacket(packet.build(StlBuffer{}));

				if (dynamic_pointer_cast<TickBox>(getRenderObject(Interface::RememberTickbox))->ticked())
					sConfig->setString("UI", "DefaultLogin", userStr.c_str());

				sApplication->clearPopups();
				sApplication->spawnTimedPopup("Connecting...", 1.0f);
			}
		}

		m_loginAttemptAt = 0;
	}

	m_topLeftCorner = { static_cast<int>(sApplication->sWf() - m_backgroundGui->getGlobalBounds().width) / 2, static_cast<int>((sApplication->sHf() - 750.f) / 3.f) };
	m_bottomRightCorner = m_topLeftCorner + sf::Vector2i(m_backgroundGui->vbounds());
	m_background->renderStretch({ 0, 0 }, { sApplication->sWf(), sApplication->sHf() });
	m_backgroundGui->render(sf::Vector2f(m_topLeftCorner));
	__super::render();
}
