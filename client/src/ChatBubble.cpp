#include "stdafx.h"
#include "ChatBubble.h"
#include "TextBox.h"
#include "ContentMgr.h"

ChatBubble::ChatBubble(RenderObject& owner, const sf::Vector2i topLeftCorner) :
	ExpandableWindow(owner, 0,
		"saybox_topright.png",
		"saybox_topacross.png",
		"saybox_topleft.png",
		"saybox_bottomright.png",
		"saybox_bottomacross.png",
		"saybox_bottomleft.png",
		"saybox_leftup.png",
		"saybox_rightup.png",
		"saybox_center.png"),
	m_width(0),
	m_height(0),
	m_textOffset({ 16, 16 })
{
	setKeepInScreen(false);
}

ChatBubble::~ChatBubble()
{

}

void ChatBubble::draw()
{
	if (m_text == nullptr)
		return;

	refreshBounds();
	ExpandableWindow::render();
	m_text->draw();	
}

void ChatBubble::setText(const string& fontname, const int characterSize, const string& str, const sf::Color color /*= sf::Color::White*/)
{
	m_text = make_unique<TextBox>(sContentMgr->getFont(fontname), characterSize);

	if (color != sf::Color::White)
		m_text->setColor(color);

	const sf::Vector2i linePosition = m_topLeftCorner + sf::Vector2i(m_textOffset);
	const sf::Vector2i finalPosition = linePosition + sf::Vector2i(0, m_height);

	m_text->setData(finalPosition.x, finalPosition.y, str, Defines::MaxBubbleWidth, TextBox::AlignCenterBounds);
	m_height = m_text->getHeight();
	m_width = m_text->getWidth();

	if (m_width < Defines::MaxBubbleWidth)
	{
		int extraMissing = Defines::MaxBubbleWidth - m_width;
		m_text->setData(finalPosition.x - (extraMissing / 2), finalPosition.y, str, Defines::MaxBubbleWidth, TextBox::AlignCenterBounds);
	}
}

void ChatBubble::moveTo(const sf::Vector2i pos)
{
	if (m_topLeftCorner == pos || m_text == nullptr)
		return;

	const auto diff = pos - m_topLeftCorner;
	m_topLeftCorner = pos;
	m_text->moveTo(m_text->getPosition() + diff);
}

void ChatBubble::refreshBounds()
{
	getBottomRightCornerRef() = m_topLeftCorner + sf::Vector2i(m_width, m_height) + m_textOffset + m_textOffset;
}
