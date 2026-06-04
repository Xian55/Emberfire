#include "stdafx.h"
#include "Inventory.h"
#include "Application.h"
#include "ItemIcon.h"
#include "Connector.h"
#include "Game.h"
#include "World.h"
#include "Equipment.h"
#include "ContentMgr.h"
#include "Text.h"
#include "ConfirmMessageBox.h"
#include "TradeWindow.h"
#include "Bank.h"

#include "..\StringHelpers.h"
#include "..\Shared\GamePacketClient.h"
#include "..\SqlConnector\QueryResult.h"

#include <assert.h>

Inventory::Inventory(World& owner, const int id) :
	WorldPanel(owner, id, "inventory.png", sf::Vector2i(300, 20))
{
	setMultiInput(true);
	setIndependant(true);

	for (int i = Interface::Slot1; i <= Interface::Slot49; ++i)
	{ 
		auto gameIcon = attachIcon(make_shared<ItemIcon>(*this, i, "gameicon40"));
		gameIcon->setEnableDragActivate(true);
		addRenderObject(gameIcon);
	}

	attachAddObj(make_shared<Button>(*this, "sort_inventory", Interface::SortButton), sf::Vector2i{257, 390});

	m_moneyDraw = make_unique<Text>(sContentMgr->getFont("Palatino Linotype Regular.ttf"));
	m_moneyDraw->setCharacterSize(15);
	m_moneyDraw->setOriginalColor(sf::Color(255, 215, 0, 255));
	m_moneyDraw->setOutlineThickness(2.f);
	m_moneyDraw->setOutlineColor(sf::Color(0, 0, 0, 50));

	setMoney(0);
}

Inventory::~Inventory()
{

}

void Inventory::render()
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

	int xPos = min(sApplication->sW() - mouseableWidth(), (sApplication->sW() / 2) + 580);
	getTopLeftCornerRef().x = xPos;

	__super::render();
	m_moneyDraw->draw(getTopLeftCornerRef().x + 53, getTopLeftCornerRef().y + 399);
}

void Inventory::input()
{
	for (auto& itr : m_slots)
		itr->setAllowRightClick(!world().isIconGrabbed());

	__super::input();

	if (auto rightClicked = dynamic_pointer_cast<ItemIcon>(popFirstRightClickButton()))
	{
		if (rightClicked->getEntry() != 0)
		{
			// if at x, try to do y, else default to inventory
			if (world().isPanelOpen(World::Interface::BankPanel))
			{
				GP_Client_MoveInventoryToBank packet;
				packet.m_from = rightClicked->getId();
				packet.m_autoSelectForMe = true;
				sConnector->sendPacket(packet.build(StlBuffer{}));
				Game::playItemSound(rightClicked->getEntry());
			}
			else if (world().isPanelOpen(World::Interface::TradeWindowPanel))
			{
				GP_Client_TradeAddItem packet;
				packet.m_invSlot = rightClicked->getId();
				sConnector->sendPacket(packet.build(StlBuffer{}));
				Game::playItemSound(rightClicked->getEntry());
			}
			else if (world().isPanelOpen(World::Interface::VendorNpcPanel))
			{
				// SELL = op12 (GROUND TRUTH, money-verified live capture): u8 slot, u16 itemId.
				GP_Client_SellItem packet;
				packet.m_slot = rightClicked->getId();
				packet.m_itemId = rightClicked->getItemDef();
				sConnector->sendPacket(packet.build(StlBuffer{}));
			}
			else
			{
				if (int spellId = rightClicked->getCastedSpellId())
				{
					auto& st = sContentMgr->db("spell_template");
					uint64_t attributes = _atoi64(st.data(spellId, "attributes").c_str());

					if (Util::maskHas(attributes, SpellDefines::Attributes::TargetsItem))
					{
						world().setItemAction(rightClicked);
						sContentMgr->playSound("alert_entry_a.ogg");
					}
					else
					{						
						GP_Client_UseItem packet;
						packet.m_slot = rightClicked->getId();
						packet.m_itemId = rightClicked->getEntry();
						sConnector->sendPacket(packet.build(StlBuffer{}));
					}
				}
				else if (int questStart = atoi(sContentMgr->db("item_template").data(rightClicked->getEntry(), "quest_offer").c_str()))
				{					
					GP_Client_UseItem packet;
					packet.m_slot = rightClicked->getId();
					packet.m_itemId = rightClicked->getEntry();
					sConnector->sendPacket(packet.build(StlBuffer{}));
				}
				else
				{
					if (rightClicked->getItemDef().m_soulbound || rightClicked->hasEquipErrors())
					{
						GP_Client_EquipItem packet;
						packet.m_slotInv = rightClicked->getId();
						packet.m_itemId = rightClicked->getItemDef();
						sConnector->sendPacket(packet.build(StlBuffer{}));
					}
					else
					{
						m_equipItemCache = rightClicked;
						sApplication->spawnPopup("This item will become Soulbound. Accept?", ConfirmMessageBox::ConfirmBox_YesNo, m_equipItemConfirmCode = ConfirmMessageBox::uniqueCode());
					}
				}
			}
		}
	}

	if (world().getGrabbedIcon() != nullptr && world().getGrabbedIcon()->getType() == GameIcon::Type::Item && world().getGrabbedIcon()->getOwner() == this)
	{
		if (world().mouseInWorld())
		{
			bool destroy = false;

			if (world().getGrabbedIcon()->wasActivatedByDragging() && world().getGrabbedIcon()->wasReleasedDragging())
				destroy = true;
			else
				destroy = sApplication->mouseUp(sf::Mouse::Left);

			if (destroy)
			{
				// Destroy Item
				sApplication->clearMouseUp();
				sApplication->spawnPopup("This will DESTROY the item. Accept?", ConfirmMessageBox::ConfirmBox_YesNo, m_destroyItemConfirmCode = ConfirmMessageBox::uniqueCode());
				m_destroyItemCache = dynamic_pointer_cast<ItemIcon>(world().getGrabbedIcon());
				world().setGrabbedIcon(nullptr);
			}
		}
	}

	if (auto confirmBox = sApplication->popConfirmBox({ m_equipItemConfirmCode, m_destroyItemConfirmCode }))
	{
		if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
		{
			if (confirmBox->getCode() == m_equipItemConfirmCode)
			{
				if (m_equipItemCache != nullptr)
				{
					// Equip
					GP_Client_EquipItem packet;
					packet.m_slotInv = m_equipItemCache->getId();
					packet.m_itemId = m_equipItemCache->getItemDef();
					sConnector->sendPacket(packet.build(StlBuffer{}));
				}
			}
			else if (confirmBox->getCode() == m_destroyItemConfirmCode)
			{
				if (m_destroyItemCache != nullptr)
				{
					// Destroy
					GP_Client_DestroyItem packet;
					packet.m_slot = m_destroyItemCache->getId();
					packet.m_itemId = m_destroyItemCache->getItemDef();
					sConnector->sendPacket(packet.build(StlBuffer{}));
					world().setGrabbedIcon(nullptr);
					sContentMgr->queueSound("alert_delete_item_a.ogg", 200);
				}
			}
		}
	}

	if (popButtonId(Interface::SortButton))
	{
		GP_Client_SortInventory packet;
		sConnector->sendPacket(packet.build(StlBuffer{}));
	}

	__super::grabIcon();
}
		
bool Inventory::overrideIconGrab(shared_ptr<GameIcon> targetIcon) /*final*/
{
	if (world().getItemAction() != nullptr)
	{
		GP_Client_UseItem packet;
		packet.m_slot = world().getItemAction()->getId();
		packet.m_itemId = world().getItemAction()->getEntry();
		packet.m_target_InvSlot = targetIcon->getId();
		sConnector->sendPacket(packet.build(StlBuffer{}));

		// Clear
		world().setItemAction(nullptr);
		return true;
	}

	if ((sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt))
		&& targetIcon->getEntry() != 0)
	{
		if (targetIcon->getStackCount() > 1)
		{
			// SplitItemStack opcode UNRESOLVED (op12 turned out to be SELL). Guard so Alt+click can't
				// drop the connection until its real opcode is captured.
				if (!GamePacket::validOpcode(Opcode::Client_SplitItemStack))
					return true;
				GP_Client_SplitItemStack packet;
			packet.m_slot = targetIcon->getId();
			sConnector->sendPacket(packet.build(StlBuffer{}));
			Game::playItemSound(targetIcon->getEntry());
		}

		return true;
	}

	return __super::overrideIconGrab(targetIcon);
}

void Inventory::setMoney(const int amount)
{	
	if (wasInputLastFrame() && amount > m_money)
	{
		int gainedMoney = amount - m_money;
		string msg = ("+" + to_string(abs(gainedMoney)));
		sf::Vector2i renderPos(getTopLeftCornerRef().x + 53, getTopLeftCornerRef().y + 399);

		world().flushCombatMsgs(0);
		world().pushCombatMessage(msg, renderPos, sf::Color(255, 215, 0, 255), false, 0.7f, 0, false, 1.f, false, true, false, true);
	}

	m_money = amount;
	m_moneyDraw->setOriginalString(Util::formatMoneyCommas(amount));
}

void Inventory::refreshTooltips()
{
	for (auto& itr : m_slots)
		itr->refreshTooltip();
}

bool Inventory::getItemStack(const int itemId, int& stackCount) const
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

shared_ptr<GameIcon> Inventory::attachIcon(shared_ptr<GameIcon> gi)
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
	
int Inventory::getFirstItemEntrySlot(const int itemEntry) const
{
	for (auto& itr : m_slots)
	{
		if (itr->getEntry() == itemEntry)
			return itr->getId();
	}

	return -1;
}

void Inventory::givenGabbedIcon(shared_ptr<GameIcon> myIcon, shared_ptr<GameIcon> heldIcon)
{
	// Inventory to Inventory is a swap
	if (heldIcon->isChildOf(*this))
	{
		Game::playItemSound(heldIcon->getEntry());

		if (myIcon != heldIcon)
		{
			GP_Client_MoveItem packet;
			packet.m_from = heldIcon->getId();
			packet.m_to = myIcon->getId();
			sConnector->sendPacket(packet.build(StlBuffer{}));
			m_darkenSlot = heldIcon->getId();
		}
		else
		{
			// Redundant
		}
	}

	// Equipment to Inventory
	else if (auto equipment = dynamic_cast<Equipment const*>(heldIcon->getOwner()))
	{
		if (myIcon->getEntry() != 0)
		{
			if (auto ptr = dynamic_pointer_cast<ItemIcon>(myIcon))
			{
				// Swapping an equipped item for another item is the same as trying to equip the one in your bag
				GP_Client_EquipItem packet;
				packet.m_slotInv = myIcon->getId();
				packet.m_itemId = ptr->getItemDef();
				sConnector->sendPacket(packet.build(StlBuffer{}));
				m_darkenSlot = myIcon->getId();
			}
		}
		else
		{
			GP_Client_UnequipItem packet;
			packet.m_equipSlot = Equipment::convertInterface(static_cast<Equipment::Interface>(heldIcon->getId()));
			packet.m_inventoryDest = myIcon->getId();
			sConnector->sendPacket(packet.build(StlBuffer{}));
			Game::playItemSound(heldIcon->getEntry());
		}
	}

	// Trade Window to Inventory
	else if (auto tradeWindow = dynamic_cast<TradeWindow const*>(heldIcon->getOwner()))
	{
		if (auto itemIcon = dynamic_pointer_cast<ItemIcon>(heldIcon))
		{
			GP_Client_TradeRemoveItem packet;
			packet.m_itemGuid = itemIcon->getItemGuid();
			sConnector->sendPacket(packet.build(StlBuffer{}));
		}
	}

	// Bank Window to Inventory
	else if (auto bankWindow = dynamic_cast<Bank const*>(heldIcon->getOwner()))
	{
		if (auto itemIcon = dynamic_pointer_cast<ItemIcon>(heldIcon))
		{
			GP_Client_UnBankItem packet;
			packet.m_slot = heldIcon->getId();
			packet.m_inventorySlot = myIcon->getId();
			packet.m_chooseInvSlotForMe = false;
			sConnector->sendPacket(packet.build(StlBuffer{}));
			m_darkenSlot = myIcon->getId();
			Game::playItemSound(heldIcon->getEntry());
		}
	}
}