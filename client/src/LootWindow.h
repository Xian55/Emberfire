#pragma once

#include "WorldPanel.h"

#include "..\Shared\ItemDefines.h"

class DraggableNode;
class GameIconList;
class ItemIcon;
class Tooltip;

class LootWindow : public WorldPanel
{
	public:
		enum Interface
		{
			ScrollUp,
			ScrollDown,
			IconList,
			TakeAll,
			TooltipScratch   // off-screen ItemIcon used only to build loot tooltips by def
		};

	public:
		LootWindow(World& owner, const int id);
		virtual ~LootWindow();

		void reset();
		void setSourceGuid(const int guid) { m_sourceGuid = guid; }
		void addItem(const ItemDefines::ItemDefinition itemId, const int count);
		void addMoney(const int amount);

		int getSourceGuid() const { return m_sourceGuid; }

		// --- read + drive the loot list from Lua (the live window stays the model; force-hidden) ---
		int  lootCount() const;
		bool lootAt(const int index, ItemDefines::ItemDefinition& def, int& stack, bool& isGold) const;
		void lootIndex(const int index);   // click slot index -> GP_Client_LootItem (item identity)
		void lootAll();                    // Take All
		void linkIndex(const int index);   // shift-click: link the item into chat
		shared_ptr<Tooltip> buildLootTooltip(const int index);

	private:
		void input() final;
		void render() final;

		int m_sourceGuid{ 0 };

		shared_ptr<Button> m_upButton;
		shared_ptr<Button> m_downButton;
		shared_ptr<Button> m_takeallButton;
		shared_ptr<GameIconList> m_gameIconList;
		shared_ptr<ItemIcon> m_tooltipIcon;   // scratch, not rendered; setItemDef + buildTooltip on demand
		unique_ptr<DraggableNode> m_dragNode;
};