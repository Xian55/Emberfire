#include "stdafx.h"
#include "DialogPanel.h"
#include "TextBoxRo.h"
#include "ContentMgr.h"
#include "Button.h"
#include "World.h"
#include "DraggableNode.h"
#include "Application.h"
#include "ClientObject.h"
#include "Text.h"
#include "ClientPlayer.h"

DialogPanel::DialogPanel(World& owner, const int id, const string& background, const sf::Vector2i& closeButtonOffset) :
	WorldPanel(owner, id, background, closeButtonOffset)
{
	setIndependant(true);

	m_dragNode = make_unique<DraggableNode>();
	m_dragNode->addAnchor(&m_topLeftCorner);
	m_dragNode->addAnchor(&m_bottomRightCorner);
	m_dragNode->setDragNodeHeight(45);
}

DialogPanel::~DialogPanel()
{

}

void DialogPanel::input()
{
	__super::input();
	
	m_dragNode->updateDraggableObject(*this);

	int xpos = world().getPanelMaxXpos();

	// May as well be zero in such a case
	if (xpos < getDefualtXPos())
		xpos = 0;

	if (m_lastWorldPanelXpos != xpos)
	{
		refreshDockPosition(xpos);
	}
	else
	{
		// Undock if grabbed far away from docking origin
		if (m_docked && m_dragNode->isGrabbed() && abs(getTopLeftCornerRef().x - xpos) > 50 && abs(getTopLeftCornerRef().y - worldPanelYPos()) > 50)
			m_docked = false;
	}

	// The panel list has collapsed, and the player was letting this panel stick to it
	// Put this ack in the center ish
	if (m_docked && xpos == 0 && m_lastWorldPanelXpos != 0 && !m_dragNode->isGrabbed())
		resetPosition();

	if (xpos == 0)
		m_docked = false;

	m_lastWorldPanelXpos = xpos;
}

void DialogPanel::refreshDockPosition(int xpos /*= -1*/)
{
	if (xpos == -1)
		xpos = max(world().getPanelMaxXpos(), getDefualtXPos());

	m_dragNode->cancelGrab();

	if (getTopLeftCornerRef().x < xpos && abs(getTopLeftCornerRef().y - worldPanelYPos()) < 50)
	{
		getTopLeftCornerRef().x = xpos;
		getTopLeftCornerRef().y = worldPanelYPos();
		m_docked = true;
	}

	if (m_docked)
	{
		getTopLeftCornerRef().x = xpos;
		getTopLeftCornerRef().y = worldPanelYPos();
	}
}

int DialogPanel::getDefualtXPos() const
{
	auto center = sApplication->centerOfScreen();
	return center.x - 480;
}

void DialogPanel::resetPosition()
{	
	getTopLeftCornerRef() = { getDefualtXPos(), worldPanelYPos() };
}

void DialogPanel::render()
{
	__super::render();

	if (m_world.myself() != nullptr && m_world.myself()->hasSpline())
	{
		if (auto npc = m_world.getClientObject(m_world.getGossipGuid()))
		{
			if (m_world.myself()->getWorldPosition().getDist(npc->getWorldPosition()) >= 5)
				m_world.closePanel(static_cast<World::Interface>(getId()));
		}
	}
}