#include "stdafx.h"
#include "Button.h"
#include "ContentMgr.h"
#include "Application.h"
#include "Sprite.h"
#include "ButtonScript.h"
#include "Tooltip.h"

#include "..\Geo2d.h"

#include <assert.h>

Button::Button(RenderObject& owner, const string& scriptName, const int id /*= 0*/,
	const sf::Vector2i position /*= sf::Vector2i()*/, SfKeyEvent activateKey /*= SfKeyEvent()*/) :
	RenderObject(&owner, id),
	m_keyEvent(activateKey),
	m_wasActivated(false),
	m_wasRightClicked(false),
	m_wasKeyboundActivated(false),
	m_playSoundFromKeyboard(false),
	m_allowRightClick(false),
	m_customizeSize(0),
	m_renderIdle(true),
	m_pressedLeft(false),
	m_highlight(false),
	m_modifierBlocked(false),
	m_pressedRight(false),
	m_flash(false),
	m_flashTimer(0.0f),
	m_keyDown(false),
	m_wasHeldDown(false)
{
	changeScript(scriptName);
	m_topLeftCorner = position;

	// All sprites of a button must be same dimensions
	ASSERT(m_spriteIdle->getGlobalBounds().height == m_spriteHover->getGlobalBounds().height && m_spriteHover->getGlobalBounds().height == m_spritePress->getGlobalBounds().height);
	ASSERT(m_spriteIdle->getGlobalBounds().width == m_spriteHover->getGlobalBounds().width && m_spriteHover->getGlobalBounds().width == m_spritePress->getGlobalBounds().width);
	assignBottomLeftCorner();
}

Button::~Button()
{

}

void Button::input()
{
	// Start-over every frame
	m_wasActivated = false;
	m_wasRightClicked = false;
	m_wasKeyboundActivated = false;
	
	if (!wasInputLastFrame())
		m_wasHeldDown = false;
	
	// Let's say you have a bind that's for Shift+5, but also a bind for just regular 5.
	// You don't want the 5 and the Shift+5 to both trigger just because the 5 was active in both cases.
	// If the main key is held but a modifier is also held, and we don't want a modifier, then do stuff
	if (sf::Keyboard::isKeyPressed(m_keyEvent.code) && sApplication->isKeyPresssChecksAllowed())
	{
		if (!m_keyEvent.alt && !m_keyEvent.shift && !m_keyEvent.control)
		{
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
				sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) ||
				sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
			{
				m_modifierBlocked = true;
			}
		}
	}
	else
	{
		m_modifierBlocked = false;
	}

	if (m_modifierBlocked)
		return;

	const bool mousedOverReal = isMousedOver(true);

	//if (!mousedOverReal && MouseableNode::isMousedOver(true))
	//	 isMousedOver(true);

	bool leftBefore = m_pressedLeft;

	if (m_allowLeftClick)
	{
		updatePressed(m_pressedLeft, m_wasActivated, mousedOverReal, sf::Mouse::Left);

		if (m_wasActivated)
			m_wasLeftClicked = true;
	}

	if (m_allowRightClick)
		updatePressed(m_pressedRight, m_wasRightClicked, mousedOverReal, sf::Mouse::Right);	
	
	if (m_pressedLeft && m_enableDragActivate)
	{
		if (m_lastDragPos == sf::Vector2i{ 0, 0 })
			m_lastDragPos = sApplication->mousePos();

		if (Geo2d::distance2di(m_lastDragPos.x, m_lastDragPos.y, sApplication->mousePos().x, sApplication->mousePos().y) > 15)
		{
			m_wasRightClicked = false;
			m_wasKeyboundActivated = false;
			m_wasLeftClicked = false;
			m_wasHeldDown = false;
			m_wasActivated = true;
			m_wasActivatedByDragging = true;
			m_wasReleasedDragging = false;

			// Prevent firing over and over
			m_pressedLeft = false;
		}
	}

	bool result = false;
	
	if (m_keyDown)
		result = sApplication->popKeyDown(m_keyEvent.code, m_keyEvent.alt, m_keyEvent.control, m_keyEvent.shift);
	else
	{
		if (m_wasHeldDown)
		{
			bool allKeysLetGo = true;

			if (m_keyEvent.alt && sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt))
				allKeysLetGo = false;

			if (m_keyEvent.control && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
				allKeysLetGo = false;

			if (m_keyEvent.shift && sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
				allKeysLetGo = false;

			if (!sf::Keyboard::isKeyPressed(m_keyEvent.code))
				allKeysLetGo = true;
			else
				allKeysLetGo = false;

			if (allKeysLetGo)
			{
				result = true;
				m_wasHeldDown = false;
				sApplication->clearKeyUp();
			}
		}
		else
		{
			m_wasHeldDown = sApplication->popKeyDown(m_keyEvent.code, m_keyEvent.alt, m_keyEvent.control, m_keyEvent.shift);
		}
	}

	if (result)
	{
		if (m_soundClick.size() > 0 && m_playSoundFromKeyboard)
			sContentMgr->playSound(m_soundClick);

		m_wasActivated = true;
		m_wasKeyboundActivated = true;
	}

	if (mousedOverReal)
	{
		// Let go after a recent fire != another activate
		if (m_wasActivated && m_enableLeftFire && !m_pressedLeft && std::clock() - m_lastLeftFire < 100)
			m_wasActivated = false;

		// Timer start on key down
		if (leftBefore != m_pressedLeft && m_pressedLeft)
			m_leftFireCoold = std::clock() + 500;

		if (m_enableLeftFire && m_pressedLeft && (int64_t)std::clock() - (int64_t)m_leftFireCoold >= 25)
		{
			m_lastLeftFire = std::clock();
			m_leftFireCoold = std::clock();
			m_wasActivated = true;
		}
	}
	else
	{
		m_leftFireCoold = 0;
	}

	// Notice goes away after clicking
	if (m_wasActivated && m_activationClearsExclaim)
		m_isExclaimNotice = false;
}

void Button::render()
{
	assignBottomLeftCorner();

	if (m_renderIdle)
	{
		if (m_customizeSize != 0)
		{
			m_spriteIdle->renderStretch({ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) },
				{ static_cast<float>(m_topLeftCorner.x) + static_cast<float>(m_customizeSize), static_cast<float>(m_topLeftCorner.y) + static_cast<float>(m_customizeSize) });
		}
		else
		{
			m_spriteIdle->render({ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) });
		}
	}

	shared_ptr<Sprite> overlay = nullptr;

	// Only render changes if we're actually accepting Input
	if ((wasInputLastFrame() || m_highlight) && !m_modifierBlocked && m_enableHighlight)
	{
		if (m_highlight)
		{
			overlay = m_spritePress;
		}
		else if (isMousedOver())
		{
			if (m_pressedLeft || m_pressedRight || m_wasHeldDown)
				overlay = m_spritePress;
			else
				overlay = m_spriteHover;
		}

		if (sApplication->isKeyHeld(m_keyEvent))// && Game::gameAllowsKeyUse(this))
			overlay = m_spritePress;
		
		if (m_flash && overlay != m_spritePress)
		{
			m_flashTimer += sApplication->delta();

			if (m_flashTimer > 0.5f)
			{
				overlay = m_spriteHover;

				if (m_flashTimer > 1.0f)
					m_flashTimer = 0.0f;
			}
		}

		if (overlay != nullptr)
		{
			if (m_customizeSize != 0)
			{
				overlay->renderStretch(	{ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) },
					{ static_cast<float>(m_topLeftCorner.x) + static_cast<float>(m_customizeSize), static_cast<float>(m_topLeftCorner.y) + static_cast<float>(m_customizeSize) });
			}
			else
			{
				overlay->render({ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) });
			}
		}
	}
			
	if (m_tooltip != nullptr && wasInputLastFrame() && isMousedOver(true))
	{
		popTooltipRefresh();
		sApplication->setTooltip(m_tooltip);
	}

	updateDragLogic();
	pumpExclaimNotice();
}

void Button::updateDragLogic()
{
	if (m_lastDragPos.x != 0 && m_lastDragPos.y != 0 && !sf::Mouse::isButtonPressed(sf::Mouse::Left))
	{		
		m_wasReleasedDragging = true;
		m_lastDragPos = sf::Vector2i{ 0, 0 };
	}
}

void Button::setExclaimNotice(const bool v)
{
	if (m_exclaimNotice == nullptr)
	{
		m_exclaimNotice = sContentMgr->spawnSprite("button_notice_teal.png");
		m_exclaimNotice->setHotspotEasy(true, true);
	}

	m_isExclaimNotice = v;
}
	
void Button::pumpExclaimNotice()
{
	if (m_isExclaimNotice)
	{
		m_exclaimNoticeYOff += sApplication->delta() * 12.f * (m_exclaimNoticeSwitcher ? -1.f : 1.f);

		if (!m_exclaimNoticeSwitcher && m_exclaimNoticeYOff > 3.f)
			m_exclaimNoticeSwitcher = true;

		if (m_exclaimNoticeSwitcher && m_exclaimNoticeYOff < -3.f)
			m_exclaimNoticeSwitcher = false;

		m_exclaimNotice->render(sf::Vector2f((float)getBottomRightCornerRef().x - 10, (float)getTopLeftCornerRef().y - m_exclaimNoticeYOff));
	}
}

void Button::assignBottomLeftCorner()
{
	if (m_customizeSize != 0)
		m_bottomRightCorner = { m_topLeftCorner.x + m_customizeSize, m_topLeftCorner.y + m_customizeSize };
	else
		m_bottomRightCorner = { m_topLeftCorner.x + static_cast<int>(m_spriteIdle->getGlobalBounds().width), m_topLeftCorner.y + static_cast<int>(m_spriteIdle->getGlobalBounds().height) };
}

void Button::updatePressed(bool& output, bool& successVariable, const bool mousedOverReal, const sf::Mouse::Button b) const
{
	if (mousedOverReal && sApplication->mouseDown(b))
	{
		output = true;
		sApplication->clearMouseDown();
	}
	else if (output && !sf::Mouse::isButtonPressed(b))
	{
		output = false;

		if (mousedOverReal)
		{
			if (m_soundClick.size() > 0)
				sContentMgr->playSound(m_soundClick);

			successVariable = true;
			sApplication->clearMouseUp();
		}
	}
}

bool Button::popResult(const bool val)
{
	if (val)
	{
		// Only one may be true after a pop
		m_wasActivated = false;
		m_wasRightClicked = false;
		m_wasKeyboundActivated = false;
		m_wasLeftClicked = false;
		return true;
	}

	return false;
}

void Button::changeScript(const string& scriptName)
{
	if (auto script = sContentMgr->buttonScript(scriptName))
	{
		m_spriteIdle = sContentMgr->spawnSprite(script->textureIdle());
		m_spriteHover = sContentMgr->spawnSprite(script->textureHover());
		m_spritePress = sContentMgr->spawnSprite(script->texturePress());
		m_soundClick = script->soundPress();
	}
	else
	{
		blog(Logger::LOG_ERROR, "Missing button script %s", scriptName.c_str());
	}
}

void Button::setTooltip(shared_ptr<Tooltip> t)
{
	m_tooltip = t;
}

int Button::getW() const
{
	return m_customizeSize ? m_customizeSize : static_cast<int>(m_spriteIdle->getGlobalBounds().width);
}

int Button::getH() const
{
	return m_customizeSize ? m_customizeSize : static_cast<int>(m_spriteIdle->getGlobalBounds().height);
}
