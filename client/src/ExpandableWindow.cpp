#include "stdafx.h"
#include "ExpandableWindow.h"
#include "Application.h"
#include "DraggableNode.h"
#include "ContentMgr.h"
#include "Sprite.h"

#include <assert.h>

ExpandableWindow::ExpandableWindow(RenderObject& owner, const int id,
	const string& textureTopRight,	const string& textureTopAcross, const string& textureTopLeft, 
	const string& textureBottomRight, const string& textureBottomAcross, 	const string& textureBottomLeft,
	const string& textureLeftUp, const string& textureRightUp, const string& textureCenter, const bool dynamic /*= true*/) : 
	RenderObject(&owner, id),
	m_keepInScreen(true)
{
	ASSERT(m_spriteTopRight = sContentMgr->spawnSprite(textureTopRight));
	ASSERT(m_spriteTopAcross = sContentMgr->spawnSprite(textureTopAcross));
	ASSERT(m_spriteTopLeft = sContentMgr->spawnSprite(textureTopLeft));
	ASSERT(m_spriteBottomRight = sContentMgr->spawnSprite(textureBottomRight));
	ASSERT(m_spriteBottomAcross = sContentMgr->spawnSprite(textureBottomAcross));
	ASSERT(m_spriteBottomLeft = sContentMgr->spawnSprite(textureBottomLeft));
	ASSERT(m_spriteLeftUp = sContentMgr->spawnSprite(textureLeftUp));
	ASSERT(m_spriteRightUp = sContentMgr->spawnSprite(textureRightUp));
	ASSERT(m_spriteCenter = sContentMgr->spawnSprite(textureCenter));

	ASSERT(m_spriteTopRight->getGlobalBounds().height == m_spriteBottomLeft->getGlobalBounds().height &&
		m_spriteTopLeft->getGlobalBounds().height == m_spriteBottomRight->getGlobalBounds().height);

	ASSERT(m_spriteTopRight->getGlobalBounds().width == m_spriteBottomLeft->getGlobalBounds().width &&
		m_spriteTopLeft->getGlobalBounds().width == m_spriteBottomRight->getGlobalBounds().width);

	if (dynamic)
	{
		m_dragNode = make_unique<DraggableNode>();
		m_dragNode->addAnchor(&m_bottomRightCorner);
	}
}

ExpandableWindow::~ExpandableWindow()
{

}

void ExpandableWindow::setChromeScale(const float s)
{
	// All nine slices scale uniformly; getCenterWidth/Height + the render math read the (now scaled)
	// getGlobalBounds(), so corners stay square and edges/center stretch to fit.
	for (auto* spr : { &m_spriteTopRight, &m_spriteTopAcross, &m_spriteTopLeft,
	                   &m_spriteBottomRight, &m_spriteBottomAcross, &m_spriteBottomLeft,
	                   &m_spriteLeftUp, &m_spriteRightUp, &m_spriteCenter })
		(*spr)->setScale(s, s);
}

void ExpandableWindow::input()
{
	MouseableNode bottomCornerNode;
	bottomCornerNode.updateBottomRightCorner(m_bottomRightCorner);
	bottomCornerNode.updateTopLeftCorner({ m_bottomRightCorner.x - 25, m_bottomRightCorner.y - 25 });

	if (m_dragNode != nullptr)
		m_dragNode->updateDraggableObject(bottomCornerNode);	
}

void ExpandableWindow::render()
{
	if (m_keepInScreen)
	{
		// Keep inside game window
		sApplication->restrictIntoWindow(m_topLeftCorner);
		sApplication->restrictIntoWindow(m_bottomRightCorner);
	}

	// Don't let the window inverse on itself
	{
		if (getCenterWidth() < 0)
			m_bottomRightCorner.x = m_topLeftCorner.x + (static_cast<int>(m_spriteTopLeft->getGlobalBounds().width) * 2);

		if (getCenterHeight() < 0)
			m_bottomRightCorner.y = m_topLeftCorner.y + (static_cast<int>(m_spriteTopLeft->getGlobalBounds().height) * 2);
	}

	const int centerWidth = getCenterWidth();
	const int centerHeight = getCenterHeight();

	// Don't let the window inverse on itself
	ASSERT(centerWidth >= 0);
	ASSERT(centerHeight >= 0);

	// Top Left
	m_spriteTopLeft->render({ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) });

	// Top Across
	m_spriteTopAcross->renderStretch(

		// Start
		{ static_cast<float>(m_topLeftCorner.x) + m_spriteTopLeft->getGlobalBounds().width, static_cast<float>(m_topLeftCorner.y) },

		// End
		{ static_cast<float>(m_topLeftCorner.x) + m_spriteTopLeft->getGlobalBounds().width + static_cast<float>(centerWidth), 
			static_cast<float>(m_topLeftCorner.y) + m_spriteTopLeft->getGlobalBounds().height });

	// Left Down
	m_spriteLeftUp->renderStretch(

		// Start
		{ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) + m_spriteTopLeft->getGlobalBounds().height },

		// End
		{ static_cast<float>(m_topLeftCorner.x) + m_spriteTopLeft->getGlobalBounds().width,
			static_cast<float>(m_topLeftCorner.y) + m_spriteTopLeft->getGlobalBounds().height + static_cast<float>(centerHeight) });

	// Center
	m_spriteCenter->renderStretch(

		// Start
		{ static_cast<float>(m_topLeftCorner.x) + m_spriteTopLeft->getGlobalBounds().width, static_cast<float>(m_topLeftCorner.y) + m_spriteTopLeft->getGlobalBounds().height },

		// End
		{ static_cast<float>(m_topLeftCorner.x) + m_spriteTopLeft->getGlobalBounds().width + static_cast<float>(centerWidth), 
			static_cast<float>(m_topLeftCorner.y) + m_spriteTopLeft->getGlobalBounds().height + static_cast<float>(centerHeight) });

	// Top Right
	m_spriteTopRight->render({ static_cast<float>(m_topLeftCorner.x) + m_spriteTopLeft->getGlobalBounds().width + static_cast<float>(centerWidth), 
		static_cast<float>(m_topLeftCorner.y) });

	// Right Down
	m_spriteRightUp->renderStretch(

		// Start
		{ static_cast<float>(m_topLeftCorner.x) + m_spriteTopLeft->getGlobalBounds().width + static_cast<float>(centerWidth), 
			static_cast<float>(m_topLeftCorner.y) + m_spriteTopLeft->getGlobalBounds().height },

		// End
		{ static_cast<float>(m_topLeftCorner.x) + (m_spriteTopLeft->getGlobalBounds().width * 2) + static_cast<float>(centerWidth),
		static_cast<float>(m_topLeftCorner.y) + m_spriteTopLeft->getGlobalBounds().height + static_cast<float>(centerHeight) });

	// Bottom Right
	m_spriteBottomRight->setOrigin({ m_spriteBottomRight->getGlobalBounds().width, m_spriteBottomRight->getGlobalBounds().height });
	m_spriteBottomRight->render({ static_cast<float>(m_bottomRightCorner.x), static_cast<float>(m_bottomRightCorner.y) });

	// Bottom Left
	m_spriteBottomLeft->setOrigin({ 0, m_spriteBottomLeft->getGlobalBounds().height });
	m_spriteBottomLeft->render({ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_bottomRightCorner.y) });

	// Bottom Across
	m_spriteBottomAcross->renderStretch(

		// Start
		{ static_cast<float>(m_topLeftCorner.x) + m_spriteBottomRight->getGlobalBounds().width, 
			static_cast<float>(m_bottomRightCorner.y) - m_spriteBottomRight->getGlobalBounds().height },

		// End
		{ static_cast<float>(m_topLeftCorner.x) + m_spriteBottomRight->getGlobalBounds().width + static_cast<float>(centerWidth), 
			static_cast<float>(m_bottomRightCorner.y) });

	m_topRightCorner.x = m_bottomRightCorner.x;
	m_topRightCorner.y = m_topLeftCorner.y;
}


int ExpandableWindow::getCenterWidth() const
{
	return m_bottomRightCorner.x - m_topLeftCorner.x - (static_cast<int>(m_spriteTopLeft->getGlobalBounds().width) * 2);
}

int ExpandableWindow::getCenterHeight() const
{
	return m_bottomRightCorner.y - m_topLeftCorner.y - (static_cast<int>(m_spriteTopLeft->getGlobalBounds().height) * 2);
}
