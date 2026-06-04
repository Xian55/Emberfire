#pragma once

#include "RenderObject.h"

class Sprite;
class Tooltip;

class Button : public RenderObject
{
	public:
		Button(RenderObject& owner, const string& scriptName, const int id = 0, const sf::Vector2i position = sf::Vector2i(), 
			SfKeyEvent activateKey = SfKeyEvent());
		virtual ~Button();
		
		void changeScript(const string& scriptName);
		void setPlayButtonOnKeyboardPress(const bool b) { m_playSoundFromKeyboard = b; }
		void setAllowRightClick(const bool b) { m_allowRightClick = b; }
		void setRenderIdle(const bool b) { m_renderIdle = b; }
		void setHighlight(const bool b) { m_highlight = b; }
		void setTooltip(shared_ptr<Tooltip> t);
		void setKeyEvent(const SfKeyEvent ke) { m_keyEvent = ke; }
		void setSoundClick(const string& name) { m_soundClick = name; }
		void setFlashing(const bool v) { m_flash = v; }
		void setKeyDown(const bool v) { m_keyDown = v; }
		void setEnableHighlight(const bool v) { m_enableHighlight = v; }
		void setEnableLeftFire(const bool v ) { m_enableLeftFire = v; }
		void setEnableDragActivate(const bool v) { m_enableDragActivate = v; }
		void setAllowLeftClick(const bool v) { m_allowLeftClick = v; }
		void setExclaimNotice(const bool v);
		void setActivated(const bool v) { m_wasActivated = true; }
		void updateDragLogic();

		// Returns true if button was pressed since last input
		bool popActivated() final { return popResult(m_wasActivated); }
		bool popRightClicked() final { return popResult(m_wasRightClicked); }
		bool popLeftClicked() final { return popResult(m_wasLeftClicked); }
		bool popKeyBound() final { return popResult(m_wasKeyboundActivated); }
		bool wasReleasedDragging() const { return m_wasReleasedDragging; }
		bool wasActivatedByDragging() const { return m_wasActivatedByDragging; }
		bool isHeldDown() const { return m_wasHeldDown; }
		bool isLeftHeld() const { return m_pressedLeft; }
		bool isExclaimNotice() const { return m_isExclaimNotice; }
		bool isEnableDragActivate() const { return m_enableDragActivate; }
		
		void resetWasReleaseActivated() { m_wasReleasedDragging = false; }
		void resetWasActivatedByDragging() { m_wasActivatedByDragging = false; }
		void resetActivated() final { m_wasActivated = true; }
		
		int getW() const;
		int getH() const;
		
		auto getTooltip() const { return m_tooltip; }
		const auto& getKeyEvent() const { return m_keyEvent; }

	protected:
		virtual void input() override;
		virtual void render() override;
		virtual void popTooltipRefresh() {}

		void pumpExclaimNotice();
		void assignBottomLeftCorner();

		bool isHighlight() const { return m_highlight; }
		
		bool m_activationClearsExclaim{true};

		string m_soundClick;

		shared_ptr<Sprite> m_spriteIdle;
		shared_ptr<Sprite> m_spriteHover;
		shared_ptr<Sprite> m_spritePress;

	private:
		void updatePressed(bool& output, bool& successVariable, const bool mousedOverReal, const sf::Mouse::Button b) const;

		bool popResult(const bool val);
		
		bool m_wasActivated;
		bool m_wasRightClicked;
		bool m_wasKeyboundActivated;
		bool m_playSoundFromKeyboard;
		bool m_allowRightClick;
		bool m_allowLeftClick{true};
		bool m_renderIdle;
		bool m_pressedLeft;
		bool m_pressedRight;
		bool m_highlight;
		bool m_modifierBlocked;
		bool m_flash;
		bool m_keyDown;
		bool m_wasHeldDown;
		bool m_enableHighlight{true};
		bool m_enableLeftFire{false};
		bool m_isExclaimNotice{false};
		bool m_exclaimNoticeSwitcher{false};		
		bool m_enableDragActivate{false};
		bool m_wasLeftClicked{false};
		bool m_wasReleasedDragging{false};
		bool m_wasActivatedByDragging{false};

		clock_t m_leftFireCoold{0};
		clock_t m_lastLeftFire{0};

		int m_customizeSize;

		float m_flashTimer;
		float m_exclaimNoticeYOff{0};

		SfKeyEvent m_keyEvent;

		sf::Vector2i m_lastDragPos;

		shared_ptr<Tooltip> m_tooltip;
		shared_ptr<Sprite> m_exclaimNotice;
};

