#include "stdafx.h"
#include "Bank.h"
#include "Application.h"
#include "ItemIcon.h"
#include "Connector.h"
#include "Game.h"
#include "World.h"
#include "Inventory.h"
#include "ContentMgr.h"
#include "ConfirmMessageBox.h"
#include "TradeWindow.h"

#include "..\StringHelpers.h"
#include "..\Shared\GamePacketClient.h"
#include "..\SqlConnector\QueryResult.h"

#include <assert.h>

Bank::Bank(World& owner, const int id) :
	DialogPanel(owner, id, "bank.png", sf::Vector2i(300, 20))
{
	setMultiInput(true);
	//setIndependant(true);

	for (int i = Interface::Slot1; i <= Interface::Slot49; ++i)
	{ 
		auto gameIcon = attachIcon(make_shared<ItemIcon>(*this, i, "gameicon40"));
		gameIcon->setEnableDragActivate(true);
		addRenderObject(gameIcon);
	}

	attachAddObj(make_shared<Button>(*this, "sort_inventory", Interface::SortButton), sf::Vector2i{257, 390});
}

Bank::~Bank()
{

}

void Bank::render()
{
	for (auto& itr : m_slots)
	{
		itr->setDarken(itr->getId() == m_darkenSlot || itr.get() == world().getItemAction());
				
		auto cd = [&](const int id)
		{
			if (id == 0)
				return;

			// 0, 0 will just mean no cooldown
			auto cd = world().getCooldown(id);		
			itr->setCooldown(cd.first, cd.second);
		};
		
		cd(itr->getCastedSpellId());
		cd(-itr->getCastedSpellCategoryCooldown());
	}

	__super::render();
}

void Bank::input()
{
	for (auto& itr : m_slots)
		itr->setAllowRightClick(!world().isIconGrabbed());

	__super::input();

	if (auto rightClicked = dynamic_pointer_cast<ItemIcon>(popFirstRightClickButton()))
	{
		if (rightClicked->getEntry() != 0)
		{
			// Right clicking an item should do 1 thing; try to remove from bank
			GP_Client_UnBankItem packet;
			packet.m_slot = rightClicked->getId();
			packet.m_chooseInvSlotForMe = true;
			sConnector->sendPacket(packet.build(StlBuffer{}));
			Game::playItemSound(rightClicked->getEntry());
		}
	}

	if (popButtonId(Interface::SortButton))
	{
		GP_Client_SortBank packet;
		sConnector->sendPacket(packet.build(StlBuffer{}));
	}

	__super::grabIcon();
}
		
bool Bank::overrideIconGrab(shared_ptr<GameIcon> targetIcon) /*final*/
{ 
	// ???
	//;

	return __super::overrideIconGrab(targetIcon);
}

void Bank::refreshTooltips()
{
	for (auto& itr : m_slots)
		itr->refreshTooltip();
}

bool Bank::getItemStack(const int itemId, int& stackCount) const
{
	for (auto& itr : m_slots)
	{
		if (itr->getEntry() == itemId)
		{
			stackCount = itr->getStackCount();
			return true;
		}
	}

	return false;
}

shared_ptr<GameIcon> Bank::attachIcon(shared_ptr<GameIcon> gi)
{
	ASSERT(Interface::Slot1 == 0);
	ASSERT(Interface::Slot49 == 48);
	ASSERT(gi->getId() <= Interface::Slot49);

	const int y = (gi->getId()) / 7;
	const int x = (gi->getId()) % 7;
	const auto offset = sf::Vector2i(27 + (45 * x), 75 + (45 * y));

	gi->setAnchor(&getTopLeftCornerRef());
	gi->setOffset(offset);
	m_slots.push_back(gi);
	return gi;
}
	
int Bank::getFirstItemEntrySlot(const int itemEntry) const
{
	for (auto& itr : m_slots)
	{
		if (itr->getEntry() == itemEntry)
			return itr->getId();
	}

	return -1;
}

void Bank::givenGabbedIcon(shared_ptr<GameIcon> myIcon, shared_ptr<GameIcon> heldIcon)
{
	// Bank to Bank is a swap
	if (heldIcon->isChildOf(*this))
	{
		Game::playItemSound(heldIcon->getEntry());

		// Redundant
		if (myIcon != heldIcon)
		{
			GP_Client_MoveBankToBank packet;
			packet.m_from = heldIcon->getId();
			packet.m_to = myIcon->getId();
			sConnector->sendPacket(packet.build(StlBuffer{}));
			m_darkenSlot = myIcon->getId();
			Game::playItemSound(heldIcon->getEntry());
		}
	}

	// Inventory to Bank
	else if (auto inventory = dynamic_cast<Inventory const*>(heldIcon->getOwner()))
	{
		if (heldIcon->getEntry() != 0)
		{
			GP_Client_MoveInventoryToBank packet;
			packet.m_from = heldIcon->getId();
			packet.m_to = myIcon->getId();
			sConnector->sendPacket(packet.build(StlBuffer{}));
			m_darkenSlot = myIcon->getId();
			Game::playItemSound(heldIcon->getEntry());
		}
	}
}