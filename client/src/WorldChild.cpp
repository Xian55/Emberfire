#include "stdafx.h"
#include "WorldChild.h"
#include "World.h"
#include "GameIcon.h"
#include "ContentMgr.h"
#include "Application.h"
#include "ClientMap.h"
#include "GameChat.h"
#include "ItemIcon.h"

WorldChild::WorldChild(RenderObjectHolder& owner, const int id, World& world) :
	RenderObjectHolder(&owner, id),
	m_world(world)
{

}

WorldChild::~WorldChild()
{

}

bool WorldChild::grabIcon()
{
	if (auto btn = popFirstButton())
	{
		if (auto gameIcon = dynamic_pointer_cast<GameIcon>(btn))
		{
			if (m_world.getGrabbedIcon() != nullptr)
			{
				// Clear it here and not inside the give function because then I'd have to write it out over and over to clear it, I think we always will want to as well
				givenGabbedIcon(gameIcon, m_world.getGrabbedIcon());
				m_world.setGrabbedIcon(nullptr);
			}
			else if (gameIcon->getEntry() != 0)
			{
				if (overrideIconGrab(gameIcon))
					return false;

				auto itemIcon = dynamic_pointer_cast<ItemIcon>(gameIcon);

				if ((sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) && itemIcon != nullptr)
				{
					if (auto gameChat = dynamic_pointer_cast<GameChat>(world().getRenderObject(World::GameChatBox)))
					{
						sContentMgr->playSound("button_click_a.ogg");
						gameChat->promptLinkAnItem(itemIcon->getItemDef());
						return true;
					}
				}
				else if (gameIcon->grab(m_world))
				{
					sContentMgr->playSound("alert_entry_a.ogg");
					return true;
				}
			}
		}
		else
		{
			// If it's not actually an icon then pretend we never popped it
			btn->resetActivated();
		}
	}
	
	if (m_world.isIconGrabbed() && m_world.getGrabbedIcon()->wasActivatedByDragging() && m_world.getGrabbedIcon()->wasReleasedDragging())
	{
		if (auto gameIcon = dynamic_pointer_cast<GameIcon>(getFirstMousedOver(true)))
		{			
			gameIcon->resetWasReleaseActivated();
			gameIcon->resetWasActivatedByDragging();
			givenGabbedIcon(gameIcon, m_world.getGrabbedIcon());				
			m_world.setGrabbedIcon(nullptr);
		}
	}

	return false;
}

bool WorldChild::depositGrabbedIconToEmptySpace() const
{
	if (!sApplication->mouseUp(sf::Mouse::Left))
		return false;

	if (world().getGrabbedIcon() == nullptr)
		return false;

	if (world().getGrabbedIcon()->getOwner() != this)
		return false;

	if (world().getMousedOverObj() == 0)
	{
		// Is this a workaround? We're probably moused over the map if this 0
		sApplication->clearMouseUp();
		return true;
	}

	auto worldMousedObj = world().getRenderObject(world().getMousedOverObj());

	if (worldMousedObj == nullptr)
		return false;

	// Clicking on the game map with an icon in hand
	if (dynamic_pointer_cast<ClientMap>(worldMousedObj) != nullptr)
	{
		sApplication->clearMouseUp();
		return true;
	}

	return false;
}
