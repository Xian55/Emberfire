#pragma once

#include "DialogPanel.h"

#include "..\Shared\GamePacketServer.h"

class GameIcon;
class ItemIcon;
class Tooltip;
class TextBoxRo;
class SpriteRo;
class Button;

class VendorNpc : public DialogPanel
{
	public:
		enum Interface
		{
			IconSlot1 = 1,
			IconSlot12 = 12,
			Title1 = 13,
			Title12 = 24,
			GoldCoin1 = 25,
			GoldCoin12 = 36,
			Cost1 = 37,
			Cost12 = 48,
			Highlight1 = 49,
			Highlight12 = 61,

			LocalMoneyText,
			RepairButton,
			BuybackButton,
			LeftBtn,
			RightBtn,
		};

	public:
		VendorNpc(World& owner, const int id);
		virtual ~VendorNpc();

		void reset();
		void setLocalMoney(const int money);
		void registerItem(const ItemDefines::ItemDefinition entry, const int cost, const int stack);
		void updateItemQuantity(const ItemDefines::ItemDefinition entry, const int stack);

		// --- read + drive for the Lua vendor view (the live window stays the model; Lua scrolls the full list) ---
		int  itemCount() const { return static_cast<int>(m_items.size()); }
		bool itemAt(int index, int& itemId, int& affix, int& cost, int& supply) const;
		void buyIndex(int index, int count);
		shared_ptr<Tooltip> buildVendorTooltip(int index);

	private:
		void input() final;
		void render() final;

		void buyItem(const ItemDefines::ItemDefinition entry, const int count);

	private:
		GP_Server_GossipMenu::VendorSlot getItemInfo(const Interface id) const;

		shared_ptr<GameIcon> attachIcon(shared_ptr<GameIcon> gi);
		shared_ptr<TextBoxRo> attachTitle(shared_ptr<TextBoxRo> txt);
		shared_ptr<SpriteRo> attachGoldCoin(shared_ptr<SpriteRo> txt);
		shared_ptr<TextBoxRo> attachCost(shared_ptr<TextBoxRo> txt);
		shared_ptr<Button> attachHighlight(shared_ptr<Button> txt);

		shared_ptr<TextBoxRo> m_localMoneyTxt;
		vector<GP_Server_GossipMenu::VendorSlot> m_items;
		map<Interface, GP_Server_GossipMenu::VendorSlot> m_legend;

		size_t m_pageNumber{0};

		ItemDefines::ItemDefinition m_popupBuyEntry;
		shared_ptr<ItemIcon> m_tooltipIcon;   // scratch, not rendered; setItemDef + buildTooltip on demand
};

