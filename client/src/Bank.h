#pragma once

#include "DialogPanel.h"

#include "..\Shared\PlayerDefines.h"

class ItemIcon;
class GameIcon;
class Text;

class Bank : public DialogPanel
{
	public:
		enum Interface
		{
			Slot1 = 0,
			Slot49 = PlayerDefines::Inventory::NumSlotsBank - 1,

			//
			SortButton
		};

	public:
		Bank(World& owner, const int id);
		virtual ~Bank();
		
		void refreshTooltips();
		void clearDrakenedSlot() { m_darkenSlot = -1; }

		int getFirstItemEntrySlot(const int itemEntry) const;

		bool getItemStack(const int itemId, int& stackCount) const;

	private:
		void input() final;
		void render() final;
		void givenGabbedIcon(shared_ptr<GameIcon> myIcon, shared_ptr<GameIcon> heldIcon) final;
		
		bool overrideIconGrab(shared_ptr<GameIcon> myIcon) final;

		shared_ptr<GameIcon> attachIcon(shared_ptr<GameIcon> gi);

		int m_darkenSlot{-1};

		vector<shared_ptr<GameIcon>> m_slots;
};

