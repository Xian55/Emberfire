#pragma once

#include "GameIcon.h"

class ItemTemplate;

class ItemIcon : public GameIcon
{
	public:
		ItemIcon(RenderObject& owner, const int id, const string& frame);

		void setShowSellPrice(const bool v) { m_showSellPrice = v; }
		void setIgnoreLevelErrorForIcon(const bool v) { m_ignoreLevelErrorForIcon = v; }
		void setIgnoreSpellbookNotifyForIcon(const bool v) { m_ignoreSpellbookNotifyForIcon = v; }
		void setItemDef(const ItemDefines::ItemDefinition def);
		void setItemGuid(const int itemGuid) { m_itemGuid = itemGuid; }

		string deduceTitle() const final;
		string deduceDescription() const final;

		int getItemGuid() const { return m_itemGuid; }
		int getCastedSpellId() const final { return m_castedSpellId; }
		int getCastedSpellCategoryCooldown() const final { return m_castedSpellCategoryCooldown; }
	
		bool hasEquipErrors() const { return m_hasEquipErrors; }

		// Build this item's tooltip and return it (the Lua view positions + asserts it, since the bag is
		// force-hidden and the icon's own hover path never runs). nullptr if the slot is empty.
		shared_ptr<Tooltip> buildTooltip();

		const auto& getItemDef() const { return m_itemDef; }

	public:		
		static sf::Color itemColor(const ItemDefines::Quality quality);
		static string formItemTitle(const ItemDefines::ItemDefinition def);

	protected:
		void pickIcon() final;
		void fillTooltip() final;
		void drawLabels() final;
		void onEntryChange() final;

	private:
		bool computeHasErrorIcon();
		bool alreadyKnownSkillbook(World* world) const;

		ItemDefines::ItemDefinition m_itemDef;

		int m_itemGuid{ 0 };
		int m_castedSpellId{0};
		int m_castedSpellCategoryCooldown{0};

		bool m_hasEquipErrors{false};
		bool m_ignoreLevelErrorForIcon{false};
		bool m_ignoreSpellbookNotifyForIcon{false};
		bool m_showSellPrice{true};

		shared_ptr<ItemTemplate> m_itemTemplate;
};

