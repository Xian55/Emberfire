#include "stdafx.h"
#include "DraggableNode.h"
#include "Application.h"

#include "..\Math.h"

DraggableNode::DraggableNode() :
	m_isGrabbed(false), 
	m_reverse(false),
	m_dragNodeHeight(25)
{
	m_dragButton = sf::Mouse::Button::Left;
}

DraggableNode::~DraggableNode()
{

}

void DraggableNode::updateDraggableObject(MouseableNode& self, const bool wholeObject /*= false*/)
{
	if (m_isGrabbed)
	{
		if (!sf::Mouse::isButtonPressed(m_dragButton))
			m_isGrabbed = false;

		float changeX = float(sApplication->mousePos(true).x - m_lastGrabPos.x);
		float changeY = float(sApplication->mousePos(true).y - m_lastGrabPos.y);

		if (m_dragSpeed != 1.f)
		{
			changeX *= m_dragSpeed;
			changeY *= m_dragSpeed;
		}

		for (auto& anchor : m_anchors)
		{
			anchor->x += int(m_reverse ? -changeX : changeX);
			anchor->y += int(m_reverse ? -changeY : changeY);
		}

		for (auto& anchor : m_anchorsGeo2d)
		{
			anchor->x += m_reverse ? -changeX : changeX;
			anchor->y += m_reverse ? -changeY : changeY;
		}

		m_lastGrabPos = sApplication->mousePos(true);

	}
	else if (self.MouseableNode::isMousedOver())
	{
		sf::Vector2i topCorner = self.getTopLeftCornerRef();
		sf::Vector2i bottomCorner = self.getBottomRightCornerRef();

		if (wholeObject || Util::cordsInBox(sApplication->mousePos(), topCorner, bottomCorner.x - topCorner.x, m_dragNodeHeight))
		{
			if (!m_isGrabbed && sApplication->mouseDown(m_dragButton))
			{
				m_isGrabbed = true;
				m_lastGrabPos = sApplication->mousePos();
			}
		}
	}
}

void DraggableNode::clearAnchors()
{
	m_anchors.clear();
	m_anchorsGeo2d.clear();
}

void DraggableNode::setLoneAnchor(sf::Vector2i* anchor)
{
	clearAnchors();
	addAnchor(anchor);
}

void DraggableNode::setLoneAnchor(Geo2d::Vector2* anchor)
{
	clearAnchors();
	addAnchor(anchor);
}
