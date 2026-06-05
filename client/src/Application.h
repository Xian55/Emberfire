#pragma once

#include "RenderObjectHolder.h"

class Game;
class Tooltip;

class Application : public RenderObjectHolder
{
	public:
		enum Ro
		{
			RoGame = 1,
			RoMapEditor,
			RoConsole,
			RoPopup,
			RoConfirmBox,
			RoTimedMsg,
			RoPopupPrompt,
			RoDbEditor,
			RoLuaRoot,        // root holder for Lua-created frames (addon layer)
		};

	public:
		// Call once, at program begin
		void begin();

		void cancel() { m_cancel = true; }
		void queueRecreateWindow() { m_recreateWindow = true; }
		void clearKeyUp(set<sf::Keyboard::Key> exceptions = {});
		void clearKeyDown(set<sf::Keyboard::Key> exceptions = {});
		void setTopMostMousedOver(const void* ptr) { m_topMostMousedOver = reinterpret_cast<decltype(m_topMostMousedOver)>(ptr); }
		void setCurrentPrompt(const void* ptr) { m_currentPromptInput = reinterpret_cast<decltype(m_currentPromptInput)>(ptr); }
		void setCurrentPrompt(const uintptr_t ptr) { m_currentPromptInput = ptr; }
		void queueCursor(const string& texturename) { m_desiredCursor = texturename; }
		void registerCanvas(sf::RenderTexture* canvas) { m_registeredCanvas = canvas; }
		void resetCanvas() { if (m_originalDPI) m_registeredCanvas = &m_window; else m_registeredCanvas = &m_baseRenderTexture; }
		void setDevRendering(const bool val);
		void audioDeviceChanged() { m_audioDeviceChanged = true; }
		void resetLastInputTime() { m_lastInput = ::clock(); }

		void setFullscreenBorderless(const bool val);
		void setTutorialStatus(const string& name, const bool val);
		void setTooltip(shared_ptr<Tooltip> t);
		void spawnTimedPopup(const string& str, const float numsecs);
		void spawnPopup(const string& str, const int type, const int code);
		void spawnPopupPrompt(const string& title, const int code, const bool numbersOnly = false, const bool allowZero = false);
		void clearMouseWheelEvent();
		void clearTextEvents();
		void clearMouseDown();
		void clearMouseUp();
		void clearPopups();
		void loadingSplash();
		void logout();
		void loadCfg();

		// Will keep this value from exceeding screen dimensions
		void restrictIntoWindow(sf::Vector2i& vect);

		bool mouseUp(const sf::Mouse::Button mb) const;
		bool mouseDown(const sf::Mouse::Button mb) const;
		
		bool isKeyUp() const { return m_keyUpEvent.code != 0; }
		bool isKeyDown() const { return m_keyDownEvent.code != 0; }
		bool isTooltipRegistered() const { return m_tooltip != nullptr; }
		bool keyDown(const sf::Keyboard::Key kb, const bool alt = false, const bool control = false, const bool shift = false) const;
		bool keyUp(const sf::Keyboard::Key kb, const bool alt = false, const bool control = false, const bool shift = false) const;
		bool popKeyDown(const sf::Keyboard::Key kb, const bool alt = false, const bool control = false, const bool shift = false);
		bool popKeyUp(const sf::Keyboard::Key kb, const bool alt = false, const bool control = false, const bool shift = false);
		bool isKeyHeld(const SfKeyEvent& ke) const;
		bool getTutorialStatus(const string& name) { return m_tutorialStatus[name]; }
		bool isKeyPresssChecksAllowed() const;
		bool isFullscreen() const { return m_isFullscreen; }
		bool isAudioDeviceChangedThisTick() const { return m_audioChangedThisTick; }
		bool isRenderingOriginalDpi() const { return m_originalDPI; }
		bool isDevMode() { return m_isDevMode; }

		int sW() const;
		int sH() const;
		
		int getCurrentFrameId() const { return m_currentFrameId; }
		int getLastFrameId() const { return m_lastFrameId; }

		float sWf() const;
		float sHf() const;

		auto& getWindow() { return m_window; }
		auto& canvas() { return *m_registeredCanvas; }

		const auto& getMouseEvent() const { return m_mouseDownEvent; }
		const auto& getMouseWheelEvent() const { return m_mouseWheelEvent; }
		const auto& getTextEvents() const { return m_textEvents; }
		const auto& getElapsed() const { return m_elapsed; }
		const auto& getTopMostMousedOver() const { return m_topMostMousedOver; }
		const auto& getCurrentPromptInput() const { return m_currentPromptInput; }
		const auto& getLastInput() const { return m_lastInput; }

		const float delta() const { return m_elapsed.asSeconds(); }

		string getMachineFingerprint() const;
		string getCacheFolderPath() const;

		sf::Vector2i centerOfScreen() const { return sf::Vector2i(sW() / 2, sH() / 2); }
		sf::Vector2i mousePos(const bool real = false) const;
		sf::Vector2f mousePosf(const bool real = false) const;

		shared_ptr<Game> game();

		unique_ptr<SfKeyEvent> popKeyDown();
		
		__time64_t timeNowMs() const;

	public:
		static Application* instance()
		{
			static Application app;
			return &app;
		}

	private:
		Application();
		~Application();

		void input() final;
		void render() final;
		
		void restart();
		void setCursor(const string& texturename);
		void registerCursor(const string& texturename);
		void onWindowsResize(const sf::Event& e); 
		void createSfmlWindow();
		void updateMousePosRaw();

		bool configureNvidia();

		bool m_mouseDown;
		bool m_focus;
		bool m_cancel;
		bool m_recreateWindow;
		bool m_promptKeysOnly;
		bool m_originalDPI = false;
		bool m_isFullscreen = false;
		bool m_audioDeviceChanged = false;
		bool m_audioChangedThisTick = false;
		bool m_isDevMode = false;

		int m_currentFrameId{0};
		int m_lastFrameId{0};

		uintptr_t m_topMostMousedOver;
		uintptr_t m_currentPromptInput;

		SfKeyEvent m_keyDownEvent;
		SfKeyEvent m_keyUpEvent;

		string m_defaultCursor;
		string m_desiredCursor;
		sf::Cursor m_cursor;

		sf::Time m_elapsed;
		sf::ContextSettings m_contextSettings;

		sf::Event::MouseButtonEvent m_mouseUpEvent;
		sf::Event::MouseButtonEvent m_mouseDownEvent;
		sf::Event::MouseWheelScrollEvent m_mouseWheelEvent;

		vector<sf::Event::TextEvent> m_textEvents;
		
		sf::Vector2i m_mousePosThisFrame;
		sf::Vector2i m_lastMouseDownPosition;

		sf::Vector2u m_savedWindowSize;
		sf::Vector2i m_savedWindowPos;

		sf::RenderWindow m_window;
		sf::RenderTexture m_baseRenderTexture;
		sf::RenderTarget* m_registeredCanvas;

		shared_ptr<Tooltip> m_tooltip;
		map<string, unique_ptr<sf::Cursor>> m_cursors;
		map<string, bool> m_tutorialStatus;
		uintptr_t m_currentCursor;
		clock_t m_lastInput = 0;
};

#define sApplication Application::instance()