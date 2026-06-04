#pragma once

#include "WorldPanel.h"

#include "..\Shared\PlayerDefines.h"

class ItemIcon;
class GameIcon;
class Text;

class Inventory : public WorldPanel
{
	public:
		enum Interface
		{
			Slot1 = 0,
			Slot49 = PlayerDefines::Inventory::NumSlots - 1,

			//
			SortButton
		};

	public:
		Inventory(World& owner, const int id);
		virtual ~Inventory();
		
		void refreshTooltips();
		void setMoney(const int amount);
		void clearDrakenedSlot() { m_darkenSlot = -1; }

		int getFirstItemEntrySlot(const int itemEntry) const;
		int getMoney() const { return m_money; }

		bool getItemStack(const int itemId, int& stackCount) const;

	private:
		void input() final;
		void render() final;
		void givenGabbedIcon(shared_ptr<GameIcon> myIcon, shared_ptr<GameIcon> heldIcon) final;
		
		bool overrideIconGrab(shared_ptr<GameIcon> myIcon) final;

		shared_ptr<GameIcon> attachIcon(shared_ptr<GameIcon> gi);

		int m_darkenSlot{-1};
		int m_money{0};
		int m_equipItemConfirmCode{0};
		int m_destroyItemConfirmCode{0};

		unique_ptr<Text> m_moneyDraw;
		vector<shared_ptr<GameIcon>> m_slots;
		
		shared_ptr<ItemIcon> m_equipItemCache;
		shared_ptr<ItemIcon> m_destroyItemCache;
};

