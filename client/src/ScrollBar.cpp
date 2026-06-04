#include "stdafx.h"
#include "ScrollBar.h"
#include "Button.h"
#include "DraggableNode.h"
#include "RenderObjectHolder.h"
#include "..\Math.h"
#include "Application.h"

ScrollBar::ScrollBar(RenderObject& owner, const string& scrollUpButton, const string& scrollDownButton, 
		const ScrollBarType scrollType, const string& scrollSquare /*= ""*/, const int id /*= 0*/) :
	RenderObjectHolder(&owner, id),
	m_scrollUpButton(make_shared<Button>(*this, scrollUpButton, Interface::ScrollUpButton)),
	m_scrollDownButton(make_shared<Button>(*this, scrollDownButton, Interface::ScrollDownButton)),
	m_scrollOffset(0),
	m_type(scrollType),
	m_dragNode(nullptr),
	m_allowMouseWheel(true),
	m_mouseWheelMouseoverOnly(false)
{
	m_maxOffset = 0;
	m_height = 0;

	if (!scrollSquare.empty())
	{
		m_scrollSquare = make_shared<Button>(*this, scrollSquare, Interface::ScrollSquare);
		m_scrollSquare->updateBottomRightCorner(sf::Vector2i(m_scrollSquare->getTopLeftCornerRef().x + m_scrollSquare->getW(), m_scrollSquare->getTopLeftCornerRef().y + m_scrollSquare->getH()));
		m_dragNode = make_shared<DraggableNode>();
		m_dragNode->addAnchor(&m_scrollSquare->getTopLeftCornerRef());
		m_dragNode->setDragNodeHeight(m_scrollSquare->getH());
	}

	setMultiInput(true);
	addRenderObject(m_scrollUpButton);
	addRenderObject(m_scrollDownButton);

	if (m_scrollSquare != nullptr)
		addRenderObject(m_scrollSquare);
}

ScrollBar::~ScrollBar()
{

}

void ScrollBar::input()
{
	if (m_dragNode != nullptr && m_scrollSquare != nullptr)
		m_dragNode->updateDraggableObject(*m_scrollSquare);

	__super::input();

	bool allowMouseWheel = m_allowMouseWheel;

	if (allowMouseWheel && m_mouseWheelMouseoverOnly)
		allowMouseWheel = isMousedOver();

	if (m_scrollUpButton->popActivated() || (allowMouseWheel && sApplication->getMouseWheelEvent().delta > 0))
	{
		if (m_type == ScrollTopDown)
		{
			if (m_scrollOffset)
				--m_scrollOffset;
		}
		else
		{
			if (m_scrollOffset < m_maxOffset)
				++m_scrollOffset;
		}
	}

	if (m_scrollDownButton->popActivated() || (allowMouseWheel && sApplication->getMouseWheelEvent().delta < 0))
	{
		if (m_type == ScrollTopDown)
		{
			if (m_scrollOffset < m_maxOffset)
				++m_scrollOffset;
		}
		else
		{
			if (m_scrollOffset)
				--m_scrollOffset;
		}
	}
}

void ScrollBar::render()
{
	if (m_type == ScrollLeftRight)
	{
		m_topLeftCorner = m_scrollDownButton->getTopLeftCornerRef();
		m_bottomRightCorner = m_scrollUpButton->getBottomRightCornerRef();
	}
	else
	{
		m_bottomRightCorner = m_scrollDownButton->getBottomRightCornerRef();
	}

	// Update before rendering so it stays inside the proper bounds
	updateScrollBarPosition();

	__super::render();

	if (m_scrollSquare == nullptr || m_dragNode == nullptr)
		return;

	m_scrollSquare->attemptRender();
}

bool ScrollBar::isGrabbed() const
{
	if (m_dragNode == nullptr)
		return false;

	return m_dragNode->isGrabbed();
}

void ScrollBar::setScrollOffset(const int offset)
{
	if (offset < 0)
		return;

	if (offset > m_maxOffset)
		return;

	m_scrollOffset = offset;
}

void ScrollBar::changeHeight(const int height)
{
	// No negative heights allowed
	ASSERT(height >= 0);
	m_height = height;
}

float ScrollBar::getScrollPercent() const
{
	if (!m_maxOffset)
		return 1.0f;

	return min(1.0f, float(m_scrollOffset) / float(m_maxOffset));
}

void ScrollBar::updateScrollBarPosition()
{
	if (m_scrollSquare == nullptr || m_dragNode == nullptr)
		return;

	const int topY = m_scrollUpButton->getTopLeftCornerRef().y + m_scrollUpButton->getH();
	const int bottomRenderY = m_scrollDownButton->getTopLeftCornerRef().y - m_scrollDownButton->getH();

	// For SCROLLBAR_LEFT_RIGHT
	const int leftX = m_scrollDownButton->getTopLeftCornerRef().x + m_scrollUpButton->getW();
	const int rightRenderX = m_scrollUpButton->getTopLeftCornerRef().x - m_scrollSquare->getW();

	// Let the drag node change position if it has it
	if (m_dragNode->isGrabbed())
	{
		// Don't divide by zero
		assert(m_height != 0);
		sf::Vector2i& scrollTopLeft = m_scrollSquare->getTopLeftCornerRef();

		if (m_type == ScrollLeftRight)
		{
			// Always stays horizontal with scroll buttons
			scrollTopLeft.y = m_scrollUpButton->getTopLeftCornerRef().y;

			// Can not exceed top/bottom
			scrollTopLeft.x = max(scrollTopLeft.x, leftX);
			scrollTopLeft.x = min(scrollTopLeft.x, rightRenderX);

			if (abs(leftX - scrollTopLeft.x) > m_height)
				scrollTopLeft.x += m_height - abs(leftX - scrollTopLeft.x);
		}
		else
		{
			// Always stays vertical with scroll buttons
			scrollTopLeft.x = m_scrollUpButton->getTopLeftCornerRef().x;

			// Can not exceed top/bottom
			scrollTopLeft.y = max(scrollTopLeft.y, topY);
			scrollTopLeft.y = min(scrollTopLeft.y, bottomRenderY);

			if (abs(topY - scrollTopLeft.y) > m_height)
				scrollTopLeft.y += m_height - abs(topY - scrollTopLeft.y);
		}

		// Apply change in position to scroll offset
		if (m_type == ScrollBottomUp)
		{
			const float ratioApplication = 1.0f - (static_cast<float>(abs(topY - scrollTopLeft.y)) / static_cast<float>(m_height));
			assert(ratioApplication <= 1.0f && ratioApplication >= 0);
			m_scrollOffset = static_cast<int>(round(static_cast<float>(m_maxOffset) * ratioApplication));
		}
		else if (m_type == ScrollTopDown)
		{
			const float ratioApplication = (static_cast<float>(abs(topY - scrollTopLeft.y)) / static_cast<float>(m_height));
			assert(ratioApplication <= 1.0f && ratioApplication >= 0);
			m_scrollOffset = static_cast<int>(round(static_cast<float>(m_maxOffset) * ratioApplication));
		}
		else if (m_type == ScrollLeftRight)
		{
			const float ratioApplication = (static_cast<float>(abs(leftX - scrollTopLeft.x)) / static_cast<float>(m_height));
			assert(ratioApplication <= 1.0f && ratioApplication >= 0);
			m_scrollOffset = static_cast<int>(round(static_cast<float>(m_maxOffset) * ratioApplication));
		}
	}

	// Otherwise, figure out an exact one
	else
	{
		if (m_type == ScrollLeftRight)
			changeHeight(abs(leftX - rightRenderX));
		else
			changeHeight(abs(topY - bottomRenderY));

		sf::Vector2i renderPos(m_scrollUpButton->getTopLeftCornerRef().x, 0);

		// No divide by zero
		const float scrollRatio = m_maxOffset != 0 ? (float(m_scrollOffset) / float(m_maxOffset)) : 0.0f;

		// The Y change depends on variable m_bBottomUp
		if (m_type == ScrollBottomUp)
		{
			renderPos.y = bottomRenderY - static_cast<int>(static_cast<float>(m_height) * scrollRatio);
		}
		else if (m_type == ScrollTopDown)
		{
			renderPos.y = topY + static_cast<int>(static_cast<float>(m_height) * scrollRatio);
		}
		else if (m_type == ScrollLeftRight)
		{
			renderPos.x = leftX + static_cast<int>(static_cast<float>(m_height) * scrollRatio);
			renderPos.y = m_scrollUpButton->getTopLeftCornerRef().y;
		}

		m_scrollSquare->updateTopLeftCorner(renderPos);
	}
}
