#include "stdafx.h"
#include "PromptBox.h"
#include "Application.h"
#include "Button.h"
#include "ContentMgr.h"

#include <assert.h>

PromptBox::PromptBox(RenderObject& owner, const int id, const string& fontName, shared_ptr<Button> enterButton, const sf::Vector2i& renderPos, const int maxWidth, const sf::Color fontColor, const bool stringBeyondRender /*= true*/) :
	TextLines(owner, id, fontName, Util::GeoBox2d(renderPos.x, renderPos.y, maxWidth, 0)),
	m_enterButton(enterButton),
	m_maxWidth(maxWidth),
	m_fontColor(fontColor),
	m_promptTimer(0),
	m_stringBeyondRender(stringBeyondRender),
	m_safetyRender(false),
	m_sfTextPrompt(sContentMgr->getFont(fontName)),
	m_maxPromptStringLen(MaxPromptString),
	m_asciiOnly(false),
	m_numbersOnly(false),
	m_emptyEnter(false),
	m_forcedDrawUpdateOnce(false)
{
	setDialogCharacterSize(m_characterSize);
	//m_box.h = m_font->GetHeight() + PROMPTBOX_DEFAULT_LINE_SPACING;
	m_topLeftCorner = renderPos;
}

PromptBox::~PromptBox()
{

}

void PromptBox::input()
{
	m_emptyEnter = false;

	TextLines::input();

	if (!isCurrentPrompt())
		return;
	
	sApplication->getWindow().setKeyRepeatEnabled(true);

	if (m_textPrompt.size() < m_maxPromptStringLen)
	{
		for (auto& itr : sApplication->getTextEvents())
		{
			const char letter = static_cast<char>(itr.unicode);

			if (validLetter(letter))
			{
				if (m_stringBeyondRender || m_sfTextPrompt.getStringWidth(*m_font, m_characterSize, false, m_textPrompt + letter + "|", true) < m_maxWidth)
					m_textPrompt.push_back(letter);
			}
		}
	}

	// Clipboard paste
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::V) && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && ::OpenClipboard(nullptr))
	{
		if (HANDLE hData = ::GetClipboardData(CF_TEXT))
		{
			if (char* pszText = static_cast<char*>(::GlobalLock(hData)))
			{
				m_textPrompt = pszText;
				::GlobalUnlock(hData);
			}
		}
		::CloseClipboard();
	}

	// Clipboard copy
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::C) && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && ::OpenClipboard(nullptr))
	{
		::EmptyClipboard();

		if (HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, m_textPrompt.size() + 1))
		{
			if (void* pData = ::GlobalLock(hMem))
			{
				memcpy(pData, m_textPrompt.c_str(), m_textPrompt.size() + 1);
				::GlobalUnlock(hMem);

				if (!::SetClipboardData(CF_TEXT, hMem))
					::GlobalFree(hMem);
			}
			else
			{
				::GlobalFree(hMem);
			}
		}

		::CloseClipboard();
	}

	if (sApplication->popKeyDown(sf::Keyboard::BackSpace))
	{
		if (!m_textPrompt.empty())
			m_textPrompt.erase(m_textPrompt.end() - 1);
	}

	// Enter Button
	if (m_enterButton != nullptr)
	{
		m_enterButton->attemptInput();

		if (m_enterButton->popActivated())
		{
			m_queuedStatement = m_textPrompt;
			m_textPrompt.clear();

			if (m_queuedStatement.empty())
				m_emptyEnter = true;
		}
	}
}

void PromptBox::render()
{
	if (m_enterButton != nullptr)
		m_enterButton->attemptRender();
	
	// Our goal is to minimize how often we call setStringMaxWidth
	bool forceChanged = false;

	string extra;
	string stringToRender = decideStringToRender();

	// No point making this configurable
	static float flashTimer = 0.5f;
	
	if (wasInputLastFrame() && isCurrentPrompt())
	{	
		const bool wasLower = m_promptTimer < flashTimer;
		m_promptTimer += sApplication->delta();
	
		if (m_promptTimer >= flashTimer)
		{
			extra = "|";
			
			if (wasLower)
				forceChanged = true;
		}
	
		if (m_promptTimer >= flashTimer * 2)
		{
			extra = " ";
			m_promptTimer = 0;
			forceChanged = true;
		}

		m_forcedDrawUpdateOnce = false;
	}
	else if (!m_forcedDrawUpdateOnce)
	{
		m_forcedDrawUpdateOnce = true;
		forceChanged = true;
		m_promptTimer = flashTimer / 2.0f;
	}

	// Avoid redundant calculations
	if (m_sfTextPrompt.getOriginalString() != stringToRender || forceChanged)
	{
		m_sfTextPrompt.setOriginalString(stringToRender);
		m_sfTextPrompt.setStringMaxWidth(stringToRender, m_maxWidth, true, true, extra);
	}

	m_sfTextPrompt.setColorIfNot(m_fontColor);
	m_sfTextPrompt.draw(m_topLeftCorner.x, m_topLeftCorner.y);
		
	if (m_scrollObject != nullptr)
		TextLines::render();
	
	// Set after done rendering.
	m_bottomRightCorner = { m_topLeftCorner.x + m_maxWidth, m_topLeftCorner.y + m_characterSize };
	
	// Is reset every frame.
	m_textPromptPrefix.clear();
}

string PromptBox::decideStringToRender() const
{
	string cpyStr;

	if (m_safetyRender)
	{
		cpyStr.resize(m_textPrompt.size());

		for (size_t i = 0; i < cpyStr.size(); ++i)
			cpyStr[i] = '*';
	}
	else
	{
		cpyStr = m_textPromptPrefix + m_textPrompt;
	}

	if (!m_prefix.empty())
		return m_prefix + cpyStr;

	return cpyStr;
}

void PromptBox::setPromptCharacterSize(const int characterSize)
{
	if (m_sfTextPrompt.getCharacterSize() == characterSize)
		return;

	ASSERT(characterSize > 0);
	m_sfTextPrompt.setCharacterSize(characterSize);
	m_forcedDrawUpdateOnce = false;
	m_characterSize = characterSize;
}

void PromptBox::setPromptMaxStrLen(const int len)
{
	if (m_maxPromptStringLen == len)
		return;

	if (len > MaxPromptString)
		m_maxPromptStringLen = MaxPromptString;
	else
		m_maxPromptStringLen = max(1, len);

	m_forcedDrawUpdateOnce = false;
}

void PromptBox::setPrefix(const string& str)
{
	if (m_prefix == str)
		return;

	if (str.size() < MaxPrefixString)
	{
		m_prefix = str;
		m_forcedDrawUpdateOnce = false;
	}
}

void PromptBox::setTextPrompt(const string& str)
{
	if (m_textPrompt == str)
		return;

	if (str.size() < m_maxPromptStringLen)
	{
		m_textPrompt = str;
		m_forcedDrawUpdateOnce = false;
	}
}

void PromptBox::setMaxWidth(const int val)
{
	if (m_maxWidth == val)
		return;

	m_maxWidth = val;
	m_forcedDrawUpdateOnce = false;
}

void PromptBox::setContent(const string& str)
{
	if (m_textPrompt == str)
		return;
	
	m_textPrompt = str;
	m_forcedDrawUpdateOnce = false;
}

bool PromptBox::isCurrentPrompt() const
{	
	return sApplication->getCurrentPromptInput() == reinterpret_cast<uintptr_t>(this);
}

bool PromptBox::validLetter(const char letter) const
{
	if (m_numbersOnly)
	{
		// Integers don't start with 0
		if (m_textPrompt.empty() && letter == '0')
		{
			if (!m_allowZero)
				return false;
		}

		return isdigit(letter) != 0;
	}

	if (m_asciiOnly)
		return isalpha(letter) != 0;

	return letter >= 32 && letter <= 126;
}

bool PromptBox::popEmptyEnter()
{
	const bool result = m_emptyEnter;
	m_emptyEnter = false;
	return result;
}

string PromptBox::popQueuedStatement()
{
	string result = m_queuedStatement;

	if (!result.empty())
		m_queuedStatement.clear();

	return result;
}