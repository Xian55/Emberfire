#include "stdafx.h"
#include "TextBoxRo.h"
#include "ContentMgr.h"
#include "Text.h"

TextBoxRo::TextBoxRo(RenderObject& owner, const int id, const string& fontName, const int w, const int characterSize, 
		const TextBox::Align algn /*= TextBox::AlignLeft*/, const bool centerHeight /*= false*/, const float thickness /*= 0.0f*/) :
	RenderObject(&owner, id),
	m_text(sContentMgr->getFont(fontName), characterSize),
	m_width(w),
	m_characterSize(characterSize),
	m_align(algn),
	m_centerHeight(centerHeight),
	m_thickness(thickness)
{
	setMouseable(false);
}

TextBoxRo::~TextBoxRo()
{

}

void TextBoxRo::setString(const string& str, const sf::Color color /*= sf::Color::White*/, const sf::Color outlineColor /*= sf::Color::White*/, const int maxLines /*= -1*/, const bool enableScroll /*= false*/)
{
	if (enableScroll)
	{
		m_text.setData(getTopLeftCornerRef().x, getTopLeftCornerRef().y, str, m_width, m_align, m_centerHeight, m_thickness, sf::Color(outlineColor));

		m_scrollMax = m_text.getNumLines() - maxLines;
		m_scrollOffset = 0;
		m_scrollDownCap = 0;
		m_scrollMaxLines = maxLines;
		m_scrollTextFull = m_text.getTextRef().getOriginalString();

		m_text.setData(0, 0, "", 0, m_align);
	}

	m_text.setData(getTopLeftCornerRef().x, getTopLeftCornerRef().y, str, m_width, m_align, m_centerHeight, m_thickness, sf::Color(outlineColor), maxLines);
	m_text.setColor(color, outlineColor);
}

void TextBoxRo::render()
{
	m_text.moveTo(getTopLeftCornerRef());
	m_text.draw();

	// The text box's actual position probably was changed by centering logic
	//m_text.syncPosition(getTopLeftCornerRef());
	getBottomRightCornerRef() = getTopLeftCornerRef() + sf::Vector2i(m_text.getWidth(), m_text.getHeight());
}

void TextBoxRo::pumpScrollOffset(const int val)
{
	if (val > m_scrollOffset)
		scrollDown();
	else if (m_scrollOffset > val)
		scrollUp();
}

void TextBoxRo::scrollUp()
{
	if (m_scrollOffset > 0)
	{
		m_scrollOffset -= 2;
		scrollDown();
	}
}

void TextBoxRo::scrollDown()
{
	if (m_scrollTextFull.empty())
		return;

	// Try to avoid recalculating this too many times
	if (m_scrollOffset >= m_scrollDownCap && m_scrollDownCap != 0)
		return;

	++m_scrollOffset;
	string str = m_scrollTextFull;

	for (int i = 0; i < m_scrollOffset; ++i)
	{
		do
		{
			str.erase(str.begin());
		} while(!str.empty() && str[0] != '\n');

		if (!str.empty() && str[0] == '\n')
			str.erase(str.begin());
	}

	int numLines = 1;

	for (auto& itr : str)
	{
		if (itr == '\n')
			++numLines;
	}

	// At the bottom
	if (numLines == 1)
	{
		// undo
		--m_scrollOffset;
		m_scrollDownCap = m_scrollOffset;
		return;
	}

	int popFront = numLines - m_scrollMaxLines;

	while (popFront > 0 && !str.empty())
	{
		if (str[str.size() - 1] == '\n')
			--popFront;

		str.pop_back();
	}

	setString(str, m_text.getTextRef().getFillColor(), m_text.getTextRef().getOutlineColor());
}