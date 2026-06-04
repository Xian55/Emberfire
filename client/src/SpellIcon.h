#pragma once

#include "GameIcon.h"

class SpellIcon : public GameIcon
{
	public:
		SpellIcon(RenderObject& owner, const int id, const string& frame, const int entry = 0);

		string deduceTitle() const final;
		string deduceDescription() const final;

		int getCastedSpellId() const final { return getEntry(); }
		int getCastedSpellCategoryCooldown() const final { return m_castedSpellCategoryCooldown; }
	
		int getSpellManaCost(ClientUnit& myself) const final;

	public:
		void setStackCount(const int stack) { m_stackAmount = stack; }
		
		bool canSpellScale() const;

		int getStackCount() const { return m_stackAmount; }
				
		static bool canSpellScale(const int entry);
		static string dispelTypeStr(const SpellDefines::DispelType type);

	protected:
		void pickIcon() final;
		void fillTooltip() final;
		void drawLabels() final;

	private:
		bool m_souldbound{0};

		int m_entry{0};
		int m_durabilityDeficit{0};
		int m_stackAmount{1};
		int m_castedSpellId{0};
		int m_castedSpellCategoryCooldown{0};
		int m_manaCost{0};
		int m_manaCostPct{0};
};

