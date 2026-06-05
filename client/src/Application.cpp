#include "stdafx.h"
#include "Application.h"
#include "ConsoleWindow.h"
#include "ContentMgr.h"
#include "TimedMessageBox.h"
#include "ConfirmMessageBox.h"
#include "Game.h"
#include "Connector.h"
#include "Tooltip.h"
#include "PopupPrompt.h"
#include "Options.h"
#include "WindowsKeybindHack.h"
#include "AudioDeviceNotifier.h"
#include "lua/LuaEngine.h"

#include "..\Shared\Md5.h"
#include "..\Shared\Config.h"
#include "..\Shared\DmystVersion.h"
#include "..\StringHelpers.h"

#include <assert.h>
#include <chrono>
#include <nvapi.h>
#include <NvApiDriverSettings.h>
#include <direct.h>
#include <filesystem>
#include <ShlObj.h>  
#include <imm.h>

#pragma comment(lib, "imm32.lib")

Application::Application() :
	RenderObjectHolder(nullptr),
	m_mouseDown(false),
	m_focus(true),
	m_cancel(false),
	m_recreateWindow(false),
	m_topMostMousedOver(0),
	m_currentPromptInput(0)
{
	resetCanvas();
	initOwner(this);
	loadCfg();

	AudioDeviceNotifier::getInstance().setCallback([]() { sApplication->audioDeviceChanged(); });
}

Application::~Application()
{ 

}

void Application::begin()
{
	// Threaded optimization must be disabled, restart program on new profile
	if (configureNvidia())
	{
		restart();
		return;
	}

	// Open the SFML window
	createSfmlWindow();

	if (!m_window.isOpen())
	{
		blog(Logger::LOG_ERROR, "Failed to open SFML window.");
		system("pause");
		return;
	}
	
	// Load data
	ContentMgr::instance()->init();

	// The default objects
	addRenderObject(make_shared<ConsoleWindow>(*this, RoConsole));

	registerCursor(m_defaultCursor = "cursor.png");
	registerCursor("cursor_loot.png");
	registerCursor("cursor_sword.png");
	registerCursor("cursor_gossip.png");
	registerCursor("cursor_grabbedicon.png");
	registerCursor("cursor_ground_action.png");

	sf::Clock c;

	// Start the game loop
	while (m_window.isOpen())
	{
		auto tbefore = std::clock();

		m_audioChangedThisTick = m_audioDeviceChanged;

		m_elapsed = c.restart();
		updateMousePosRaw();
		sContentMgr->update();

		auto ttook = std::clock() - tbefore;

		if (ttook > 10)
			blog(Logger::LOG_DEBUG, "sContentMgr->update took %dms", ttook);

		tbefore = std::clock();

		// Event processing
		input();

		// Lua addon layer: run OnUpdate(self, dt) handlers + drain OnClick (after input, before render).
		sLua->onFrame(m_elapsed.asSeconds());

		ttook = std::clock() - tbefore;

		if (ttook > 10)
			blog(Logger::LOG_DEBUG, "input(); took %dms", ttook);

		tbefore = std::clock();

		// Clear the whole canvas 
		m_window.clear();

		ttook = std::clock() - tbefore;

		if (ttook > 10)
			blog(Logger::LOG_DEBUG, "m_canvas.clear(); took %dms", ttook);

		tbefore = std::clock();

		// Draw stuff to the canvas
		render();

		ttook = std::clock() - tbefore;

		if (ttook > 10)
			blog(Logger::LOG_DEBUG, "render(); took %dms", ttook);

		tbefore = std::clock();

		// We're done drawing to the canvas
		m_window.display();

		if (m_desiredCursor.empty())
		{
			setCursor(m_defaultCursor);
		}
		else
		{
			setCursor(m_desiredCursor);
			m_desiredCursor.clear();
		}
		
		if (m_cancel)
		{
			m_window.close();
		}
		else if (m_recreateWindow)
		{
			m_recreateWindow = false;
			createSfmlWindow();
		}
		else if (m_audioChangedThisTick)
		{
			sContentMgr->onAudioDeviceChanged();
			m_audioDeviceChanged = false;
			m_audioChangedThisTick = false;
		}
	}
}

void Application::input()
{
	// Reset key data before checking for new input.
	//	If an event doesn't happen then it's left as this reset data.
	clearKeyUp();
	clearKeyDown();
	clearMouseDown();
	clearMouseUp();
	clearMouseWheelEvent();
	clearTextEvents();

	sf::Event e;

	// Process events
	while (m_window.pollEvent(e))
	{
		switch (e.type)
		{
			case sf::Event::Resized:
			{
				onWindowsResize(e);
				break;
			}
			case sf::Event::Closed:
			{
				m_window.close();
				return;
			}
			case sf::Event::GainedFocus:
			{
				m_focus = true;
				break;
			}
			case sf::Event::LostFocus:
			{
				m_focus = false;
				break;
			}
			case sf::Event::KeyPressed:
			{
				m_lastInput = ::clock();
				m_keyDownEvent = e.key;
				break;
			}
			case sf::Event::MouseButtonPressed:
			{
				m_lastInput = ::clock();
				m_mouseDownEvent = e.mouseButton;
				break;
			}
			case sf::Event::KeyReleased:
			{
				m_lastInput = ::clock();
				m_keyUpEvent = e.key;
				break;
			}
			case sf::Event::MouseButtonReleased:
			{
				m_lastInput = ::clock();
				m_mouseUpEvent = e.mouseButton;
				break;
			}
			case sf::Event::MouseWheelScrolled:
			{
				m_lastInput = ::clock();
				m_mouseWheelEvent = e.mouseWheelScroll;
				break;
			}
			case sf::Event::TextEntered:
			{
				m_lastInput = ::clock();

				if (e.text.unicode != '`')
					m_textEvents.push_back(e.text);
				break;
			}
		}
	}

#ifdef DMYST_INTERNAL
	if (popKeyUp(sf::Keyboard::Tilde))
	{
		if (auto consoleWindow = getRenderObject(RoConsole))
			consoleWindow->setHidden(!consoleWindow->isHidden());
		return;
	}
#endif

#ifndef DMYST_INTERNAL
	if (m_isDevMode && popKeyUp(sf::Keyboard::Tilde))
	{
		if (auto consoleWindow = getRenderObject(RoConsole))
			consoleWindow->setHidden(!consoleWindow->isHidden());
		return;
	}
#endif

	if (m_currentPromptInput != 0)
	{
		clearKeyUp({ sf::Keyboard::Escape, sf::Keyboard::Enter, sf::Keyboard::BackSpace, sf::Keyboard::Tab });
		clearKeyDown({ sf::Keyboard::Enter, sf::Keyboard::BackSpace, sf::Keyboard::Tab });
	}

	// If this is set to true again by the end of input() then it will take effect in time for next windows poll
	m_window.setKeyRepeatEnabled(false);
	m_currentPromptInput = 0;
	
	if (m_focus)
		__super::input();
	
	m_lastFrameId = m_currentFrameId;
	++m_currentFrameId;

	if (mouseDown(sf::Mouse::Left))
	{
		if (!m_mouseDown)
			m_lastMouseDownPosition = m_mousePosThisFrame;

		m_mouseDown = true;
	}
	else
	{
		m_mouseDown = false;
	}

	sKeybindHack->validate();
}

void Application::render()
{
	m_topMostMousedOver = 0;

	if (m_originalDPI)
	{
		__super::render();

		// Don't render it indefinitely. If it wants to be rendered again, it will assign itself again.
		if (m_tooltip != nullptr)
		{
			m_tooltip->draw();
			m_tooltip = nullptr;
		}
	}
	else
	{
		m_baseRenderTexture.clear();

		__super::render();

		if (m_tooltip != nullptr)
		{
			m_tooltip->draw();
			m_tooltip = nullptr;
		}

		m_baseRenderTexture.display();
		m_baseRenderTexture.setSmooth(true);
		sf::RectangleShape rectangle;
		rectangle.setTexture(&m_baseRenderTexture.getTexture());
		rectangle.setTextureRect(sf::IntRect(0, 0, m_baseRenderTexture.getSize().x, m_baseRenderTexture.getSize().y));
		rectangle.setSize({ (float)m_window.getSize().x, (float)m_window.getSize().y });
		m_window.draw(rectangle);
	}
}

bool Application::isKeyPresssChecksAllowed() const
{
	return m_currentPromptInput == 0;
}

void Application::clearKeyUp(set<sf::Keyboard::Key> exceptions /*= {}*/)
{
	for (auto& itr : exceptions)
	{
		if (m_keyUpEvent.code == itr)
			return;
	}

	m_keyUpEvent.clear();
}

void Application::clearKeyDown(set<sf::Keyboard::Key> exceptions /*= {}*/)
{
	for (auto& itr : exceptions)
	{
		if (m_keyDownEvent.code == itr)
			return;
	}

	m_keyDownEvent.clear();
}

void Application::updateMousePosRaw()
{
	m_mousePosThisFrame = sf::Mouse::getPosition(m_window);
	m_mousePosThisFrame.x += 1;
}

void Application::registerCursor(const string& texturename)
{
	if (auto texture = sContentMgr->getTexture(texturename))
	{
		sf::Image cursorImage = texture->copyToImage();

		unique_ptr<sf::Cursor> c = make_unique<sf::Cursor>();
		c->loadFromPixels(cursorImage.getPixelsPtr(), cursorImage.getSize(), sf::Vector2u{});

		m_cursors[texturename] = move(c);
	}
}

void Application::setCursor(const string& texturename)
{	
	if (auto c = m_cursors[texturename].get())
	{
		// Avoid redundant spam
		if (m_currentCursor == reinterpret_cast<uintptr_t>(c))
			return;

		m_currentCursor = reinterpret_cast<uintptr_t>(c);
		m_window.setMouseCursor(*c);
	}
}

void Application::clearPopups()
{
	destroyObjectById(RoPopup);
	destroyObjectById(RoTimedMsg);
	sLua->clearTimedPopups();   // timed toasts are rendered by the Lua default UI now
}

void Application::setTooltip(shared_ptr<Tooltip> t)
{
	m_tooltip = t;
}
		
__time64_t Application::timeNowMs() const 
{ 
	return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count(); 
}

void Application::spawnTimedPopup(const string& str, const float numsecs)
{
#ifdef DMYST_NO_LOADING_POPUP
	// Skip the long "Loading" fallback overlays (300s) that gate the UI until dismissed/timeout.
	// Short info popups (Connecting.../errors, ~3s) still show.
	if (numsecs >= 60.0f)
		return;
#endif
	// Rendered by the Lua default UI (Popups.lua) instead of the C++ TimedMessageBox.
	sLua->showTimedPopup(str, numsecs);
}

void Application::spawnPopup(const string& str, const int type, const int code)
{
	addRenderObject(make_shared<ConfirmMessageBox>(*this, RoPopup, code, str, ConfirmMessageBox::ConfirmBoxType(type)));
}

void Application::spawnPopupPrompt(const string& title, const int code, const bool numbersOnly /*= false*/, const bool allowZero /*= false*/)
{
	addRenderObject(make_shared<PopupPrompt>(*this, RoPopupPrompt, title, PopupPrompt::Codes(code), numbersOnly, allowZero));
}

void Application::restrictIntoWindow(sf::Vector2i& vect)
{
	vect.x = max(0, vect.x);
	vect.x = min(sW(), vect.x);
	vect.y = max(0, vect.y);
	vect.y = min(sH(), vect.y);
}

void Application::clearMouseWheelEvent()
{
	m_mouseWheelEvent.delta = 0;
	m_mouseWheelEvent.x = 0;
	m_mouseWheelEvent.y = 0;
}

void Application::clearTextEvents()
{
	m_textEvents.clear();
}

void Application::clearMouseDown()
{
	m_mouseDownEvent.button = sf::Mouse::Button::ButtonCount;
	m_mouseDownEvent.x = 0;
	m_mouseDownEvent.y = 0;
}

void Application::clearMouseUp()
{
	m_mouseUpEvent.button = sf::Mouse::Button::ButtonCount;
	m_mouseUpEvent.x = 0;
	m_mouseUpEvent.y = 0;
}

// Not intended to be fast
void Application::loadingSplash()
{
	sf::Texture texture;
	
	if (!texture.loadFromFile(imageFile(ImageId::LoadingSplash)))
		return;

	sf::Sprite sprite;
	sprite.setTexture(texture);

	const float x = float(((float)m_window.getSize().x / 2.f) - (texture.getSize().x / 2.0f));
	const float y = float(((float)m_window.getSize().y / 2.f) - (texture.getSize().y / 2.0f));
	sprite.setPosition({ ceil(x), ceil(y) });

	m_window.clear();
	m_window.draw(sprite);
	m_window.display();
}

void Application::logout()
{
	auto game = dynamic_pointer_cast<Game>(getRenderObject(Application::Ro::RoGame));
	
	if (game == nullptr)
	{
		blog(Logger::LOG_ERROR, "Tried to logout but game does not exist\n");
		return;
	}

	sContentMgr->stopAllLoopSound();
	loadingSplash();
	sConnector->cancel();
	game->setStage(Game::Ro::RoLogin);
	clearPopups();
	spawnPopup("Disconnected", ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);
}


string Application::getCacheFolderPath() const
{
	static string result;

	if (result.empty())
	{
		char appDataPath[MAX_PATH];

		// Try to use %APPDATA%\Emberfire\config.ini
		if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath)))
		{
			std::string appDataDir = std::string(appDataPath) + "/Emberfire/";
			
			try
			{
				if (!std::filesystem::exists(appDataDir))
					std::filesystem::create_directories(appDataDir);

				result = appDataDir;
			}
			catch (const std::filesystem::filesystem_error&)
			{
				result = "cache/";
			}
		}
		else
		{
			result = "cache/";
		}
	}

	return result;
}

void Application::loadCfg()
{
	bool localPath = true;
	std::string configPath = "config.ini"; 

#ifndef DMYST_INTERNAL
	char appDataPath[MAX_PATH];

	// Try to use %APPDATA%\Emberfire\config.ini
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath)))
	{
		std::string appDataDir = std::string(appDataPath) + "\\Emberfire";
		std::string appDataConfig = appDataDir + "\\config.ini";

		if (std::filesystem::exists(appDataConfig))
		{
			// AppData config exists, use it
			configPath = appDataConfig;
			localPath = false;
		}
		else
		{
			// Doesn't exist, try to copy local config there
			try
			{
				std::filesystem::create_directories(appDataDir);

				if (std::filesystem::exists("config.ini"))
				{
					std::filesystem::copy_file("config.ini", appDataConfig);
					configPath = appDataConfig;
					localPath = false;
				}
			}
			catch (const std::filesystem::filesystem_error&){}
		}
	}
#endif

	sConfig->setSource(configPath.c_str(), localPath);
	sConfig->registerCache(Options::Interface::System_CtmTick, "System", "CtmTick");
	sConfig->registerCache(Options::Interface::System_StickyTick, "System", "StickyTick");
	sConfig->registerCache(Options::Interface::System_EnemyNameplateTick, "System", "EnemyNameplateTick");
	sConfig->registerCache(Options::Interface::System_FriendlyNameplateTick, "System", "FriendlyNameplateTick");
	sConfig->registerCache(Options::Interface::System_YourNameTick, "System", "YourNameTick");
	sConfig->registerCache(Options::Interface::System_PlayerRanksTick, "System", "PlayerRanksTick");
	sConfig->registerCache(Options::Interface::System_ShowPlayerNameTick, "System", "ShowPlayerNameTick");
	sConfig->registerCache(Options::Interface::System_ShowNpcNameTick, "System", "ShowNpcNameTick");
	sConfig->registerCache(Options::Interface::System_MusicVolumeBar, "System", "MusicVolumeBar");
	sConfig->registerCache(Options::Interface::System_SfxVolumeBar, "System", "SfxVolumeBar");

	m_tutorialStatus["Skillbook"] = sConfig->getBool("Tutorial", "Skillbook", false);

	m_isDevMode = sConfig->getBool("System", "DevMode", false);
}

bool Application::mouseUp(const sf::Mouse::Button mb) const
{
	if (!m_focus)
		return false;

	if (mb == -1)
		return false;

	return m_mouseUpEvent.button == mb;
}

bool Application::mouseDown(const sf::Mouse::Button mb) const
{
	if (!m_focus)
		return false;

	if (mb == -1)
		return false;

	return m_mouseDownEvent.button == mb;
}

bool Application::keyDown(const sf::Keyboard::Key kb, const bool alt /*= false*/, const bool control /*= false*/, const bool shift /*= false*/) const
{
	if (!m_focus)
		return false;

	if (kb == -1)
		return false;

	return m_keyDownEvent.code == kb && m_keyDownEvent.alt == alt && m_keyDownEvent.control == control && m_keyDownEvent.shift == shift;
}

bool Application::keyUp(const sf::Keyboard::Key kb, const bool alt /*= false*/, const bool control /*= false*/, const bool shift /*= false*/) const
{
	if (!m_focus)
		return false;

	if (kb == -1)
		return false;

	return m_keyUpEvent.code == kb && m_keyUpEvent.alt == alt && m_keyUpEvent.control == control && m_keyUpEvent.shift == shift;
}

bool Application::popKeyDown(const sf::Keyboard::Key kb, const bool alt /*= false*/, const bool control /*= false*/, const bool shift /*= false*/)
{
	if (keyDown(kb, alt, control, shift))
	{
		clearKeyDown();
		return true;
	}

	return false;
}

bool Application::popKeyUp(const sf::Keyboard::Key kb, const bool alt /*= false*/, const bool control /*= false*/, const bool shift /*= false*/)
{
	if (keyUp(kb, alt, control, shift))
	{
		clearKeyUp();
		return true;
	}

	return false;
}

unique_ptr<SfKeyEvent> Application::popKeyDown()
{
	if (!m_focus)
		return nullptr;

	if (m_keyDownEvent.code == -1)
		return nullptr;

	auto result = make_unique<SfKeyEvent>(m_keyDownEvent);
	clearKeyUp();
	return result;
}

bool Application::isKeyHeld(const SfKeyEvent& ke) const
{
	if (!isKeyPresssChecksAllowed())
		return false;

	if (!sf::Keyboard::isKeyPressed(ke.code))
		return false;

	return sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) == ke.alt &&
		sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) == ke.shift &&
		sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) == ke.control;
}
		
void Application::setDevRendering(const bool val) 
{ 
	m_originalDPI = val;
	sConfig->setInt("Window", "OriginalDPI", (int)val);
}

void Application::setTutorialStatus(const string& name, const bool val)
{
	sConfig->setInt("Tutorial", name.c_str(), val);
	m_tutorialStatus[name] = val;
}

void Application::onWindowsResize(const sf::Event& e)
{
	unsigned int windowW = e.size.width;
	unsigned int windowH = e.size.height;

	float scale = 1080.f / (float)windowH;

	scale = std::min(scale, 1.f);  
	scale = std::max(scale, 0.25f);

	unsigned int renderW = (unsigned int)(windowW * scale);
	unsigned int renderH = (unsigned int)(windowH * scale);

	sf::Vector2u currentSize = m_baseRenderTexture.getSize();
	if (currentSize.x != renderW || currentSize.y != renderH)
		m_baseRenderTexture.create(renderW, renderH, m_contextSettings);

	m_baseRenderTexture.setView(sf::View(sf::FloatRect(0.f, 0.f, (float)renderW, (float)renderH)));
	m_window.setView(sf::View(sf::FloatRect(0.f, 0.f, (float)windowW, (float)windowH)));

	sConfig->setInt("Window", "Width", windowW);
	sConfig->setInt("Window", "Height", windowH);
}

void Application::createSfmlWindow()
{
	if (m_window.isOpen())
		m_window.close();

	m_originalDPI = sConfig->getBool("Window", "OriginalDPI", false);
	resetCanvas();

	m_isFullscreen = sConfig->getBool("Window", "Fullscreen", false);
	m_contextSettings.antialiasingLevel = 8;

	std::string windowName = sConfig->getString("Window", "WindowName", "Emberfire");
	windowName += " r" + to_string(DYMST_VERSION);

	int Width, Height;

	if (m_isFullscreen)
	{
		sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
		Width = desktop.width;
		Height = desktop.height;

		// Save windowed size for when they exit fullscreen
		m_savedWindowSize.x = max(sConfig->getInt("Window", "Width", 1280), 1280);
		m_savedWindowSize.y = max(sConfig->getInt("Window", "Height", 720), 720);
	}
	else
	{
		Width = max(sConfig->getInt("Window", "Width", 1280), 1280);
		Height = max(sConfig->getInt("Window", "Height", 720), 720);
	}

	sf::VideoMode videoMode(Width, Height);

	float scale = 1080.f / (float)Height;
	scale = std::min(scale, 1.f);
	scale = std::max(scale, 0.25f);
	unsigned int renderW = (unsigned int)(Width * scale);
	unsigned int renderH = (unsigned int)(Height * scale);

	m_baseRenderTexture.create(renderW, renderH, m_contextSettings);

	if (m_isFullscreen)
	{
		// Create as Default, then strip decorations
		m_window.create(videoMode, windowName.c_str(), sf::Style::Default, m_contextSettings);
		m_window.setVerticalSyncEnabled(false);
		ImmAssociateContext(m_window.getSystemHandle(), NULL);

		HWND hwnd = m_window.getSystemHandle();
		LONG style = GetWindowLong(hwnd, GWL_STYLE);
		style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
		SetWindowLong(hwnd, GWL_STYLE, style);

		SetWindowPos(hwnd, NULL, 0, 0, Width, Height, SWP_FRAMECHANGED | SWP_NOZORDER);
	}
	else
	{
		int windowflags = sf::Style::Default;

		if (sConfig->getBool("Window", "Resize", true) == false)
			windowflags &= ~sf::Style::Resize;

		m_window.create(videoMode, windowName.c_str(), windowflags, m_contextSettings);
		m_window.setVerticalSyncEnabled(false);
		ImmAssociateContext(m_window.getSystemHandle(), NULL);
	}

	m_window.setFramerateLimit(sConfig->getInt("Window", "MaxFPS", 300));
	m_window.setVerticalSyncEnabled(false);

	m_baseRenderTexture.setView(sf::View(sf::FloatRect(0.f, 0.f, (float)renderW, (float)renderH)));
	m_window.setView(sf::View(sf::FloatRect(0.f, 0.f, (float)Width, (float)Height)));

	sKeybindHack->hook(m_window);

	m_window.getSystemHandle();

	sf::Image image;
	ASSERT(image.loadFromFile(imageFile(ImageId::Icon)));
	m_window.setIcon(image.getSize().x, image.getSize().y, image.getPixelsPtr());

	loadingSplash();
}

void Application::setFullscreenBorderless(const bool val)
{
	if (m_isFullscreen == val)
		return;

	m_currentCursor = 0;
	m_isFullscreen = val;
	sConfig->setInt("Window", "Fullscreen", (int)val);

	if (m_isFullscreen)
	{
		// Save windowed size/position for restore
		m_savedWindowSize = m_window.getSize();
		m_savedWindowPos = m_window.getPosition();

		// Get the monitor the window is currently on
		HWND hwnd = m_window.getSystemHandle();
		HMONITOR hmon = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(hmon, &mi);

		int monX = mi.rcMonitor.left;
		int monY = mi.rcMonitor.top;
		int monW = mi.rcMonitor.right - mi.rcMonitor.left;
		int monH = mi.rcMonitor.bottom - mi.rcMonitor.top;

		// Create as Default, then strip decorations
		m_window.create(sf::VideoMode(monW, monH), "Emberfire", sf::Style::Default, m_contextSettings);
		m_window.setVerticalSyncEnabled(false);
		ImmAssociateContext(m_window.getSystemHandle(), NULL);

		hwnd = m_window.getSystemHandle();
		LONG style = GetWindowLong(hwnd, GWL_STYLE);
		style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
		SetWindowLong(hwnd, GWL_STYLE, style);

		SetWindowPos(hwnd, NULL, monX, monY, monW, monH, SWP_FRAMECHANGED | SWP_NOZORDER);
	}
	else
	{
		// Restore windowed
		sf::VideoMode windowed(m_savedWindowSize.x, m_savedWindowSize.y);
		m_window.create(windowed, "Emberfire", sf::Style::Default, m_contextSettings);
		m_window.setVerticalSyncEnabled(false);
		ImmAssociateContext(m_window.getSystemHandle(), NULL);
		m_window.setPosition(m_savedWindowPos);
	}

	// Re-apply settings
	m_window.setFramerateLimit(sConfig->getInt("Window", "MaxFPS", 300));
	m_window.setVerticalSyncEnabled(false);

	// Trigger resize logic
	sf::Event fakeResize;
	fakeResize.type = sf::Event::Resized;
	fakeResize.size.width = m_window.getSize().x;
	fakeResize.size.height = m_window.getSize().y;
	onWindowsResize(fakeResize);

	// Re-hook input, re-set icon, etc.
	sKeybindHack->hook(m_window);

	sf::Image image;
	image.loadFromFile(imageFile(ImageId::Icon));
	m_window.setIcon(image.getSize().x, image.getSize().y, image.getPixelsPtr());
}

void Application::restart()
{
	char buff[FILENAME_MAX];
	::_getcwd(buff, FILENAME_MAX);
	string current_working_dir(buff);
	
	string dir = Util::fmtStr("%s\\%s", current_working_dir.c_str(), "\\bin\\");
	string exepath = dir + sConfig->getString("Window", "WindowName", "Application") + ".exe";
	
	if (::SetCurrentDirectoryA(dir.c_str()))
	    ::ShellExecuteA(NULL, "open", exepath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
}

// Creates "Application Profile" with "Threaded Optimization" disabled
// https://stackoverflow.com/questions/36959508/nvidia-graphics-driver-causing-noticeable-frame-stuttering
bool Application::configureNvidia()
{
	NvAPI_Status status;

    status = NvAPI_Initialize();
    
	if (status != NVAPI_OK)
		return false;

	struct AutoNvDRSSessionHandle
	{
		~AutoNvDRSSessionHandle() { if (hSession != NULL) NvAPI_DRS_DestroySession(hSession); }
		NvDRSSessionHandle hSession{NULL};
	};

	AutoNvDRSSessionHandle h;

    status = NvAPI_DRS_CreateSession(&h.hSession);
    
	if (status != NVAPI_OK)
		return false;

    status = NvAPI_DRS_LoadSettings(h.hSession);
    
	if (status != NVAPI_OK)
		return false;

	string programname = sConfig->getString("Window", "WindowName", "Application");

	const wchar_t*  profileName             = Util::toUtf16(Util::fmtStr("%s.exe", programname.c_str())).c_str();
	const wchar_t*  appName                 = Util::toUtf16(Util::fmtStr("%s.exe", programname.c_str())).c_str();
	const wchar_t*  appFriendlyName         = Util::toUtf16(Util::fmtStr("%s.exe", programname.c_str())).c_str();
	const bool      threadedOptimization    = false;

	auto SetNVUstring = [](NvAPI_UnicodeString& nvStr, const wchar_t* wcStr)
	{
		for (int i = 0; i < NVAPI_UNICODE_STRING_MAX; i++)
			nvStr[i] = 0;

		int i = 0;
		while (wcStr[i] != 0)
		{
			nvStr[i] = wcStr[i];
			i++;
		}
	};

    // Fill Profile Info
    NVDRS_PROFILE profileInfo;
    profileInfo.version  = NVDRS_PROFILE_VER;
    profileInfo.isPredefined = 0;
    SetNVUstring(profileInfo.profileName, profileName);

    // Create Profile
    NvDRSProfileHandle hProfile;
    status = NvAPI_DRS_CreateProfile(h.hSession, &profileInfo, &hProfile);
    
	if (status != NVAPI_OK)
		return false;

    // Fill Application Info
    NVDRS_APPLICATION app;
    app.version = NVDRS_APPLICATION_VER_V1;
    app.isPredefined = 0;
    SetNVUstring(app.appName, appName);
    SetNVUstring(app.userFriendlyName, appFriendlyName);
    SetNVUstring(app.launcher, L"");
    SetNVUstring(app.fileInFolder, L"");

    // Create Application
    status = NvAPI_DRS_CreateApplication(h.hSession, hProfile, &app);
    
	if (status != NVAPI_OK)
		return false;

    // Fill Setting Info
    NVDRS_SETTING setting;
    setting.version = NVDRS_SETTING_VER;
    setting.settingId = OGL_THREAD_CONTROL_ID;
    setting.settingType = NVDRS_DWORD_TYPE;
    setting.settingLocation = NVDRS_CURRENT_PROFILE_LOCATION;
    setting.isCurrentPredefined = 0;
    setting.isPredefinedValid = 0;
    setting.u32CurrentValue = threadedOptimization ? OGL_THREAD_CONTROL_ENABLE : OGL_THREAD_CONTROL_DISABLE;
    setting.u32PredefinedValue = threadedOptimization ? OGL_THREAD_CONTROL_ENABLE : OGL_THREAD_CONTROL_DISABLE;

    // Set Setting
    status = NvAPI_DRS_SetSetting(h.hSession, hProfile, &setting);
    
	if (status != NVAPI_OK)
		return false;

    // Apply (or save) our changes to the system
    status = NvAPI_DRS_SaveSettings(h.hSession);
   
	if (status != NVAPI_OK)
		return false;

    // Success
	return true;
}

sf::Vector2i Application::mousePos(const bool real) const
{
	// Developer rendering has no limits on scale/view size etc
	if (m_originalDPI)
	{
		if (!real)
		{
			if (m_mouseDown)
				return m_lastMouseDownPosition;
		}

		return m_mousePosThisFrame;
	}
	
	sf::Vector2i pos = (real || !m_mouseDown)
		? m_mousePosThisFrame
		: m_lastMouseDownPosition;

	float scaleX = (float)m_baseRenderTexture.getSize().x / (float)m_window.getSize().x;
	float scaleY = (float)m_baseRenderTexture.getSize().y / (float)m_window.getSize().y;

	return sf::Vector2i((int)(pos.x * scaleX), (int)(pos.y * scaleY));
}

sf::Vector2f Application::mousePosf(const bool real /*= false*/) const
{
	auto pos = mousePos(real);
	return sf::Vector2f(static_cast<float>(pos.x), static_cast<float>(pos.y));
}

shared_ptr<Game> Application::game()
{
	// When this function is called, a valid pointer is expected.
	auto result = dynamic_pointer_cast<Game>(getRenderObject(RoGame));
	ASSERT(result != nullptr);
	return result;
}

int Application::sW() const
{
	if (m_originalDPI)
	{
		if (!m_window.isOpen())
			return 0;

		return static_cast<int>(m_window.getSize().x);
	}

	return static_cast<int>(m_registeredCanvas->getSize().x);
}

int Application::sH() const
{
	if (m_originalDPI)
	{
		if (!m_window.isOpen())
			return 0;

		return static_cast<int>(m_window.getSize().y);
	}

	return static_cast<int>(m_registeredCanvas->getSize().y);
}

float Application::sWf() const
{
	return float(sW());
}

float Application::sHf() const
{
	return float(sH());
}

string Application::getMachineFingerprint() const
{
	static string result;

	if (!result.empty())
		return result;

	ostringstream out;

	// Username
	char user[256] = {};
	DWORD userLen = sizeof(user);
	GetUserNameA(user, &userLen);

	// Computer name
	char computer[256] = {};
	DWORD compLen = sizeof(computer);
	GetComputerNameA(computer, &compLen);

	// Processor count
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// CPU vendor + model
	int cpuInfo[4] = {};
	__cpuid(cpuInfo, 0);
	char vendor[13];
	memcpy(vendor + 0, &cpuInfo[1], 4);
	memcpy(vendor + 4, &cpuInfo[3], 4);
	memcpy(vendor + 8, &cpuInfo[2], 4);
	vendor[12] = 0;

	// CPU brand string (gives you "AMD Ryzen 7 5800X" etc)
	char brand[49] = {};
	__cpuid(cpuInfo, 0x80000002);
	memcpy(brand, cpuInfo, 16);
	__cpuid(cpuInfo, 0x80000003);
	memcpy(brand + 16, cpuInfo, 16);
	__cpuid(cpuInfo, 0x80000004);
	memcpy(brand + 32, cpuInfo, 16);

	// Memory
	MEMORYSTATUSEX ms = {};
	ms.dwLength = sizeof(ms);
	GlobalMemoryStatusEx(&ms);
	DWORD memRounded = (DWORD)((ms.ullTotalPhys / (1024ULL * 1024 * 512)) * 512);

	out << user << "|"
		<< computer << "|"
		<< si.dwNumberOfProcessors << "|"
		<< vendor << "|"
		<< brand << "|"
		<< memRounded;

	result = MD5{}.digestString(out.str());
	return result;
}