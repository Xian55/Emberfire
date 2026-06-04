#include "stdafx.h"
#include "TextLines.h"
#include "Application.h"
#include "ContentMgr.h"
#include "ScrollBar.h"
#include "PromptBox.h"
#include "Tooltip.h"

#include "..\StringHelpers.h"

// A single line
TextLine::TextLine(const int characterSize, shared_ptr<sf::Font> f, const string& s, const sf::Color colr /*= White*/) :
	m_text(f)
{
	ASSERT(characterSize > 0);
	m_text.setCharacterSize(characterSize);
	m_text.setOriginalString(s);
	m_text.setOriginalColor(colr);
}

// The list of lines
TextLines::TextLines(RenderObject& owner, const int id, const string& fontName, const Util::GeoBox2d& box, const int fontSize /*= 12*/) :
	RenderObject(&owner, id),
	m_clickableLines(false),
	m_box(box),
	m_characterSize(fontSize),
	m_clickedLineId(-1),
	m_lineHovering(-1),
	m_clickableMouseButton(sf::Mouse::Left),
	m_maxLines(DialogWindowMaxLines)
{
	ASSERT(m_font = sContentMgr->getFont(fontName));
}

TextLines::~TextLines()
{

}

void TextLines::input()
{
	if (m_scrollObject != nullptr)
	{
		m_scrollObject->attemptInput();

		if (m_lines.size() > 0)
			m_scrollObject->setMaxOffset(m_lines.size() - 1);
	}

	if (m_clickableLines && m_lineHovering >= 0)
	{
		if (sApplication->mouseUp(m_clickableMouseButton))
		{
			m_clickedLineId = m_lineHovering;
			sApplication->clearMouseUp();
		}
	}
}

void TextLines::render()
{
	if (m_scrollObject != nullptr)
		m_scrollObject->attemptRender();
		
	// Populate a list so we can perform logic on each line if we want
	vector<TextLine*> linesToRender;
	
	if (m_lines.size() > 0)
	{
		int heightOffset = 0;
		const auto scrollOffset = getScrollOffset();

		if (m_scrollObject != nullptr && m_scrollObject->isBottomUp())
		{
			for (int i = static_cast<int>(m_lines.size()) - 1 - scrollOffset; i >= 0 && heightOffset < m_box.h; --i)
			{
				m_lines[i]->m_renderPos = sf::Vector2i(m_topLeftCorner.x + m_box.x, m_topLeftCorner.y + m_box.y + m_box.h - heightOffset);
				m_lines[i]->m_idxCache = i;
				linesToRender.push_back(m_lines[i].get());
				heightOffset += m_characterSize + m_leading;
			}
		}
		else
		{
			for (int i = scrollOffset; i < static_cast<int>(m_lines.size()) && heightOffset < m_box.h; ++i)
			{
				m_lines[i]->m_renderPos = sf::Vector2i(m_topLeftCorner.x + m_box.x, m_topLeftCorner.y + m_box.y + heightOffset);
				m_lines[i]->m_idxCache = i;
				linesToRender.push_back(m_lines[i].get());
				heightOffset += m_characterSize + m_leading;
			}
		}
	}

	// Used with m_clickableLines so that only 1 line can be true to
	m_lineHovering = -1;

	// Render the lines
	for (size_t it = 0; it < linesToRender.size(); ++it)
	{
		auto ptr = linesToRender[it];
		bool highlight = false;

		ptr->m_text.restoreColor();

		if (!m_selectedLine.empty() && ptr->m_text.getOriginalString() == m_selectedLine)
			highlight = true;

		if (wasInputLastFrame() && m_clickableLines && m_lineHovering == -1 &&
			Util::cordsInBox(sApplication->mousePos(), ptr->m_renderPos, static_cast<int>(ptr->m_text.getGlobalBounds().width), m_characterSize))
		{
			highlight = true;

			if (!ptr->m_tooltipStr.empty())
			{
				auto tooltipPosition = ptr->m_renderPos;
				tooltipPosition.x += static_cast<int>(ptr->m_text.getGlobalBounds().width) + 5;

				// Don't make more than once since it's kind of slow to do so
				if (ptr->m_tooltip == nullptr)
				{
					ptr->m_tooltip = make_shared<Tooltip>(*this, tooltipPosition);
					ptr->m_tooltip->addLine(FontId::Arial, 15, ptr->m_tooltipStr);
				}
				else
				{
					ptr->m_tooltip->moveTo(tooltipPosition);
				}

				sApplication->setTooltip(ptr->m_tooltip);
			}

			m_lineHovering = ptr->m_idxCache;
		}

		if (highlight)
		{
			sf::Color originalColor = ptr->m_text.getFillColor();

			// Can't get any brighter, so turn yellow instead
			if (originalColor == sf::Color::White)
				originalColor = sf::Color::Yellow;

			float lightenFactor = 0.2f;

			if (sf::Mouse::isButtonPressed(m_clickableMouseButton))
				lightenFactor = 0.6f;

			ptr->m_text.setColorIfNot(Util::brightenColor(originalColor, lightenFactor));
		}

		ptr->m_text.setShadowOffset(m_shadowOffset);
		ptr->m_text.setStringMaxWidth(ptr->m_text.getOriginalString(), m_box.w);
		ptr->m_text.draw(ptr->m_renderPos.x, ptr->m_renderPos.y);
	}
}

bool TextLines::isMousedOver(const bool useRealPosition /* = false */)
{
	if (m_scrollObject != nullptr && m_scrollObject->isMousedOver(useRealPosition))
		return true;

	return __super::isMousedOver(useRealPosition);
}

void TextLines::setSelectedLine(const string& txt)
{
	m_selectedLine = txt; 
	
	for (size_t i = 0; i < m_lines.size(); ++i)
	{
		if (m_lines[i]->getTextStr() == txt)
		{
			m_selectedLineId = static_cast<int>(i);
			return;
		}
	}
}

void TextLines::removeLine(const int idx)
{
	if (idx < 0 || static_cast<size_t>(idx) >= m_lines.size())
		return;

	m_lines.erase(m_lines.begin() + idx);
}

void TextLines::addLine(string line, const sf::Color color /*= White*/, uintptr_t* prunedLineAddr /*= nullptr*/)
{
	if (line.empty())
	{
		blog(Logger::LOG_ERROR, "Tried to add an empty line to TextLines object");
		return;
	}

	//Util::strReplaceAll(line, "%", "%%");
	m_lines.push_back(make_shared<TextLine>(m_characterSize, m_font, line, color));

	// Erase oldest entry at exceed max length
	if (m_lines.size() > m_maxLines)
	{
		if (prunedLineAddr != nullptr)
			*prunedLineAddr = reinterpret_cast<uintptr_t>(m_lines[0].get());

		m_lines.erase(m_lines.begin());
	}
}

void TextLines::addLines(vector<string>& lines)
{
	for (size_t i = 0; i < lines.size(); ++i)
		addLine(lines[i]);
}

void TextLines::setDialogCharacterSize(const int size)
{
	m_characterSize = size;

	// The sf::Text need to be recreated, SFML sucks.
	for (size_t i = 0; i < m_lines.size(); ++i)
	{
		const auto str = m_lines[i]->m_text.getOriginalString();
		const auto colr = m_lines[i]->m_text.getFillColor();
		m_lines[i] = make_shared<TextLine>(m_characterSize, m_font, str, colr);
	}
}

int TextLines::popPendingClickedLineId()
{
	if (m_clickedLineId == -1)
		return -1;

	auto result = m_clickedLineId;
	m_clickedLineId = -1;
	return result;
}

int TextLines::getScrollOffset() const
{
	int result = m_extraScrollOffset;

	if (m_scrollObject != nullptr)
		result += m_scrollObject->getScrollOffset();

	return result;
}

TextLine* TextLines::getLine(const int idx)
{
	if (idx < 0 || idx >= (int)m_lines.size())
		return nullptr;

	return m_lines[idx].get();
}

string TextLines::popPendingClickedLine()
{
	auto lineId = popPendingClickedLineId();

	if (lineId < 0 || lineId >= (int)m_lines.size())
		return "";

	return m_lines[lineId]->m_text.getOriginalString();
}