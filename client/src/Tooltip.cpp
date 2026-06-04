#include "stdafx.h"
#include "Tooltip.h"
#include "TextBox.h"
#include "ContentMgr.h"
#include "Text.h"
#include "TooltipGlyph.h"

Tooltip::Tooltip(RenderObject& owner, const sf::Vector2i topLeftCorner) :
	ExpandableWindow(owner, 0,
		"tooltip_topright.png",
		"tooltip_topacross.png",
		"tooltip_topleft.png",
		"tooltip_bottomright.png",
		"tooltip_bottomacross.png",
		"tooltip_bottomleft.png",
		"tooltip_leftup.png",
		"tooltip_rightup.png",
		"tooltip_center.png"),
	m_width(0),
	m_height(0),
	m_textOffset({ 16, 16 })
{
	getTopLeftCornerRef() = topLeftCorner;
}

Tooltip::~Tooltip()
{

}

void Tooltip::draw()
{
	if (m_lines.empty())
		return;
	
	auto topleftBefore = m_topLeftCorner;
	AnchoredPosition::updateAnchoredPosition();

	// This logic is nessecary because we don't want to migrate the text's position unless there was a change
	if (topleftBefore != m_topLeftCorner)
	{
		auto topleftAfter = m_topLeftCorner;
		m_topLeftCorner = topleftBefore;
		moveTo(topleftAfter);
	}

	getBottomRightCornerRef() = m_topLeftCorner + sf::Vector2i(m_width, m_height) + m_textOffset + m_textOffset;
	ExpandableWindow::render();

	for (size_t i = 0; i < m_lines.size(); ++i)
	{
		auto& itr = m_lines[i];

		itr->draw();

		if (!m_glyphs.empty())
			drawGlyph(getLineGlyph(i), itr->getPosition());
	}
}
		
shared_ptr<TooltipGlyph> Tooltip::getLineGlyph(const size_t idx) const
{
	auto itr = m_glyphs.find(idx);

	if (itr != m_glyphs.end())
		return itr->second;

	return nullptr;
}

void Tooltip::drawGlyph(const shared_ptr<TooltipGlyph> glyph, const sf::Vector2i pos) const
{
	if (glyph == nullptr)
		return;

	glyph->draw(pos, m_width);
}
		
void Tooltip::addGlyph(const shared_ptr<TooltipGlyph> glyph)
{
	if (glyph == nullptr)
		return;

	size_t idx = m_lines.size();
	m_glyphs[idx] = glyph;
	addLine(FontId::Arial, glyph->getHeight(), TextBox::getStringKey(TextBox::Key_ReplaceSpaceToMaxWidth));
}

int Tooltip::addLine(const string& fontname, const int characterSize, const string& str, const sf::Color color, const bool incrementHeight /*= true*/)
{
	auto text = make_unique<TextBox>(sContentMgr->getFont(fontname), characterSize);
	text->getTextRef().setShadowOffset(m_shadowOffset);

	if (color != sf::Color::White)
		text->setColor(color);

	const sf::Vector2i firstLinePosition = m_topLeftCorner + sf::Vector2i(m_textOffset);
	sf::Vector2i thisLinePosition = firstLinePosition + sf::Vector2i(0, m_height);

	if (m_hasMultiLineLine)
	{
		for (auto& itr : m_lines)
		{
			// Allows for multiple "lines" on the same line (multi-colored lines)
			if (itr->getPosition().y == thisLinePosition.y)
				thisLinePosition.x += itr->getWidth();
		}
	}

	text->setData(thisLinePosition.x, thisLinePosition.y, str, m_maxWidth, TextBox::AlignLeft);

	if (m_allowMoreWidthIfLastLineIsUnderThisValue != 0 && text->getLastLineWidth() < m_allowMoreWidthIfLastLineIsUnderThisValue && text->getNumLines() > 1)
	{
		text->clear();
		text->setData(thisLinePosition.x, thisLinePosition.y, str, m_maxWidth + m_allowMoreWidthIfLastLineIsUnderThisValue + 10, TextBox::AlignLeft);
	}
	
	int thisWidth = text->getWidth();

	if (m_hasMultiLineLine)
	{
		for (auto& itr : m_lines)
		{
			// Allows for multiple "lines" on the same line (multi-colored lines)
			if (itr->getPosition().y == thisLinePosition.y)
				thisWidth += itr->getWidth();
		}
	}
	
	if (incrementHeight)
		m_height += text->getNumLines() * (text->getCharacterSize() + 2) + 3;
	else
		m_hasMultiLineLine = true;

	m_width = max(m_width, thisWidth);
	m_lines.push_back(move(text));
	return thisWidth;
}

int prevX = 0;

void Tooltip::moveTo(const sf::Vector2i& pos)
{
	if (m_topLeftCorner == pos)
		return;

	prevX = pos.x;

	const auto diff = pos - m_topLeftCorner;
	m_topLeftCorner = pos;
	
	for (auto& itr : m_lines)
		itr->moveTo(itr->getPosition() + diff);
}
