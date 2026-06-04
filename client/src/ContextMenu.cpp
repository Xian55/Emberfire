#include "stdafx.h"
#include "ContextMenu.h"
#include "TextLines.h"
#include "ContentMgr.h"
#include "Application.h"
#include "ScrollBar.h"
#include "Button.h"

ContextMenu::ContextMenu(RenderObjectHolder& owner, const int id, const int reportToId, sf::Vector2i topLeftCorner) :
	ExpandableWindow(owner, id,
		"tooltip_topright.png",
		"tooltip_topacross.png",
		"tooltip_topleft.png",
		"tooltip_bottomright.png",
		"tooltip_bottomacross.png",
		"tooltip_bottomleft.png",
		"tooltip_leftup.png",
		"tooltip_rightup.png",
		"tooltip_center.png"),
	m_height(Define::CtxTextOffset * 2),
	m_width(0),
	m_reportToId(reportToId)
{
	constexpr int fontSize = 15;

	getTopLeftCornerRef() = topLeftCorner;
	m_lines = make_unique<TextLines>(*this, 0, FontId::Helvetica, Util::GeoBox2d(0, 0, Define::CtxWidth, Define::CtxHeight), fontSize);
	m_lines->setClickableLines(true);
	m_lines->setLeading(10);
	m_lines->setAnchor(&getTopLeftCornerRef());
	m_lines->setOffset(sf::Vector2i(Define::CtxTextOffset, Define::CtxTextOffset));
}

ContextMenu::~ContextMenu()
{

}

void ContextMenu::input() /*final*/
{
	getBottomRightCornerRef() = getTopLeftCornerRef() + sf::Vector2i(m_width, m_height);

	m_lines->attemptInput();

	bool scrollbarGrabbed = m_scrollBar != nullptr && m_scrollBar->isGrabbed();
	auto popResult = m_lines->popPendingClickedLine();

	// Any right click, escape, or any left click out of bounds triggers this
	if (!popResult.empty() || sApplication->mouseUp(sf::Mouse::Right) || sApplication->popKeyUp(sf::Keyboard::Escape) ||
		(sApplication->mouseUp(sf::Mouse::Left) && !MouseableNode::isMousedOver() && !scrollbarGrabbed))
	{
		if (!popResult.empty())
			sContentMgr->playSound(SfxId::ButtonClick);
		else
			sContentMgr->playSound(SfxId::WindowTargetClose);

		m_wasActivated = true;
		sApplication->clearMouseUp();

		if (auto owner = dynamic_cast<RenderObjectHolder*>(getOwner()))
			owner->unregisterCtxMenu(getId(), m_reportToId, popResult);
	}
	
	if (m_scrollBar != nullptr)
	{
		m_scrollBar->setAllowMousewheel(true);
		m_scrollBar->attemptInput();
		m_lines->setExtraScrollOfset(m_scrollBar->getScrollOffset());
	}
}

void ContextMenu::render() /*final*/
{
	if (getTopLeftCornerRef().y + m_height >= sApplication->sH())
		getTopLeftCornerRef().y -= m_height;

	ExpandableWindow::render();
	m_lines->attemptRender();

	if (m_scrollBar != nullptr)
		m_scrollBar->attemptRender();
}
		
void ContextMenu::setMaxLines(const int val)
{
	m_lines->setMaxLines(val);
}

void ContextMenu::addLine(const string& str, const sf::Color color /*= sf::Color::White*/)
{
	if (str.empty())
		return;

	// Add
	m_lines->addLine(str, color);

	// Update bounds
	auto thisLine = m_lines->getLine(m_lines->getNumLines() - 1);
	m_width = max(m_width, static_cast<int>(thisLine->getTextBounds().width) + (Define::CtxTextOffset * 2));

	int maxLines = m_lines->getBounds().h / (m_lines->getLeading() + m_lines->getCharacterSize());

	if (m_lines->getNumLines() > maxLines)
	{		
		if (m_scrollBar == nullptr)
			m_width += 100;

		// Fine to just overwrite existing one
		m_scrollBar = make_shared<ScrollBar>(*this, "scrollbar_thin_blank", "scrollbar_thin_blank", ScrollBar::ScrollTopDown, "scrollbar_thin_bright");
		m_scrollBar->setMaxOffset(m_lines->getNumLines() - maxLines);
		
		m_scrollBar->getScrollUpButton()->setAnchor(&m_topLeftCorner);
		m_scrollBar->getScrollUpButton()->setOffset(sf::Vector2i(m_width - m_scrollBar->getScrollSquare()->mouseableWidth() - Define::CtxTextOffset, Define::CtxTextOffset));

		m_scrollBar->getScrollDownButton()->setAnchor(&m_topLeftCorner);
		m_scrollBar->getScrollDownButton()->setOffset(sf::Vector2i(m_scrollBar->getScrollUpButton()->getTopLeftCornerRef().x, m_height - m_scrollBar->getScrollSquare()->mouseableHeight() - Define::CtxTextOffset));
	}
	else
	{
		m_height += thisLine->getCharacterSize() + m_lines->getLeading();
	}
}

bool ContextMenu::popResult(bool& v)
{
	const bool result = v;
	v = false;
	return result;
}