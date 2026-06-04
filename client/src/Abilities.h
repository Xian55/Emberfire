#pragma once

#include "WorldPanel.h"

#include "..\Shared\GamePacketServer.h"

class GameIconList;
class ScrollBar;
class SelectionButtons;

class Abilities : public WorldPanel
{
	public:
		enum Interface
		{
			IconList,
			ListScrollBar,
			TabSelection
		};

		enum Stage
		{
			Miscbook = 0,
			Spellbook,
		};

	public:
		Abilities(World& owner, const int id);
		virtual ~Abilities();
		
		void updateData(const int spellId, const uint8_t level, const vector<pair<int16_t, int16_t>>& bpoints);
		void setData(const vector<GP_Server_Spellbook::SpellSlot>& spells);
		void refreshTooltips();
		void toggleSpendingPoints(const bool v);
		void setStage(const Stage stage);

		bool hasSpell(const int spellId) const;
		bool getSpellPoints(const int spellId, GP_Server_Spellbook::SpellSlot& output) const;

		const auto& getDesiredInvestments() const { return m_desiredInvestments; }

	private:
		void input() final;
		void render() final;

		void requestTheorySpell(const int spellId, const int level) const;

		int getSpellLevel(const int spellId) const;

		int m_lastScrollVal{0};
		
		bool m_spendingPoints{false};

		Stage m_stage;

		map<int, int> m_desiredInvestments;

		shared_ptr<ScrollBar> m_scrollBar;
		shared_ptr<GameIconList> m_gameIconList;
		shared_ptr<SelectionButtons> m_viewChoices;

		map<Stage, vector<GP_Server_Spellbook::SpellSlot>> m_spells;
};

