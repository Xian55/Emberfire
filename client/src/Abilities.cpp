#include "stdafx.h"
#include "Abilities.h"
#include "Application.h"
#include "GameIcon.h"
#include "World.h"
#include "ContentMgr.h"
#include "GameIconList.h"
#include "ScrollBar.h"
#include "SelectionButtons.h"
#include "SpellIcon.h"
#include "ClientPlayer.h"
#include "Equipment.h"
#include "Connector.h"

#include "..\Shared\SpellDefines.h"
#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\GamePacketClient.h"

#include <assert.h>

Abilities::Abilities(World& owner, const int id) :
	WorldPanel(owner, id, "abilities.png", sf::Vector2i(412, 28)),
	m_lastScrollVal(0)
{
	setMultiInput(true);

	m_gameIconList = make_shared<GameIconList>(world(), *this, Interface::IconList,
		GameIcon::Type::Spell,
		6,
		300,
		sf::Vector2i(18, 14),
		"abilities_slot",
		75,
		fontFile(FontId::Ringbearer),
		15,
		sf::Color(168, 155, 137, 255),
		sf::Color(0, 0, 0, 64),
		2.f,
		sf::Vector2i(52, -5));

	m_gameIconList->setLowercaseText(true);
	m_gameIconList->enableDescriptions(fontFile(FontId::Palatino), 12, sf::Color(111, 99, 79, 255), sf::Color(0, 0, 0, 60), 2.f, sf::Vector2i(52, 15), 300, 2);
	m_gameIconList->setAllowDraggingIcons(true);
	attachObj(m_gameIconList, { 25, 126 });

	addRenderObject(m_gameIconList); 
	addRenderObject(m_scrollBar = make_shared<ScrollBar>(*this, "scrollbar_thin_blank", "scrollbar_thin_blank", ScrollBar::ScrollTopDown, "scrollbar_thin", Interface::ListScrollBar));
	
	m_viewChoices = make_shared<SelectionButtons>(*this, Interface::TabSelection);
	auto tab1 = make_shared<Button>(*m_viewChoices, "equipment_view", Stage::Spellbook);
	auto tab2 = make_shared<Button>(*m_viewChoices, "equipment_view", Stage::Miscbook);
	m_viewChoices->addRenderObject(attachObj(tab1, { 133, 70 }));
	m_viewChoices->addRenderObject(attachObj(tab2, { 243, 70 }));
	addRenderObject(m_viewChoices);

	setStage(Stage::Spellbook);
	m_viewChoices->setChosen(Stage::Spellbook);
}

Abilities::~Abilities()
{

}

void Abilities::input()
{
	m_scrollBar->setPos(sf::Vector2i(440, 123) + m_topLeftCorner);
	m_scrollBar->getScrollUpButton()->setPos(sf::Vector2i(440, 123) + m_topLeftCorner);
	m_scrollBar->getScrollDownButton()->setPos(sf::Vector2i(440, 502) + m_topLeftCorner);

	__super::input();

	if (m_lastScrollVal != m_scrollBar->getScrollOffset() && m_world.getGrabbedIcon() != nullptr && m_world.getGrabbedIcon()->getOwner() == m_gameIconList.get())
		m_world.setGrabbedIcon(nullptr);

	m_lastScrollVal = m_scrollBar->getScrollOffset();

	m_scrollBar->setAllowMousewheel(MouseableNode::isMousedOver(true));
	
	if (!m_spendingPoints)
		m_gameIconList->WorldChild::grabIcon();

	if (m_spendingPoints)
	{
		auto plusIcon = m_gameIconList->popClickedDongle("spend_exp_x");
		auto minusIcon = m_gameIconList->popClickedDongle("spend_exp_min");

		if (plusIcon.first != nullptr)
		{
			++m_desiredInvestments[plusIcon.first->getEntry()];
			requestTheorySpell(plusIcon.first->getEntry(), getSpellLevel(plusIcon.first->getEntry()) + m_desiredInvestments[plusIcon.first->getEntry()]);
			setStage(m_stage);
			refreshTooltips();
			world().computePendingLevelupCost();
		}
		else if (minusIcon.first != nullptr)
		{
			--m_desiredInvestments[minusIcon.first->getEntry()];
			requestTheorySpell(minusIcon.first->getEntry(), getSpellLevel(minusIcon.first->getEntry()) + m_desiredInvestments[minusIcon.first->getEntry()]);
			setStage(m_stage);
			refreshTooltips();
			world().computePendingLevelupCost();
		}
	}
}

void Abilities::render()
{
	if (m_viewChoices->getChosen() != m_stage)
		setStage(Stage(m_viewChoices->getChosen()));

	m_scrollBar->setHidden(m_gameIconList->numEntries() <= 6);
	m_scrollBar->setMaxOffset(m_gameIconList->numEntries() - m_gameIconList->numVerticalElements());
	m_gameIconList->setScroll(m_scrollBar->getScrollOffset());

	m_gameIconList->clearDarkenedEntries();
	
	// Hide the nessecary +'s and -'s during levelup process
	if (m_spendingPoints)
	{
		m_gameIconList->clearDongleExceptions();

		for (auto& itr : m_spells[m_stage])
		{
			if (!SpellIcon::canSpellScale(itr.spellId) || (m_desiredInvestments[itr.spellId] + itr.level >= SpellDefines::Misc::MaxSpellLevel))
				m_gameIconList->pushDongleException("spend_exp_x", itr.spellId);

			if (m_desiredInvestments[itr.spellId] == 0)
				m_gameIconList->pushDongleException("spend_exp_min", itr.spellId);

			// Darken any that we've never chosen
			if (m_desiredInvestments[itr.spellId] == 0 && itr.level <= 1)
				m_gameIconList->registerDarkenedEntry(itr.spellId);
		}
	}

	__super::render();

	if (__super::grabIcon())
	{
		;
	}
}
	
void Abilities::requestTheorySpell(const int spellId, const int level) const
{
	GP_Client_ReqTheoreticalSpell pk;
	pk.m_spellId = spellId;
	pk.m_level = level;
	sConnector->sendPacket(pk.build(StlBuffer{}));
}
		
int Abilities::getSpellLevel(const int spellId) const
{
	for (auto& itr : m_spells)
	{
		for (auto& subItr : itr.second)
		{
			if (subItr.spellId == spellId)
				return subItr.level;
		}
	}

	return 0;
}

void Abilities::toggleSpendingPoints(const bool v)
{
	if (m_spendingPoints == v)
		return;

	m_spendingPoints = v;
	m_desiredInvestments.clear();
	setStage(Stage(m_viewChoices->getChosen()));
}

void Abilities::setStage(const Stage stage)
{
	if (m_stage != stage)
		m_scrollBar->setScrollOffset(0);

	m_stage = stage;
	m_gameIconList->clearEntries();
	m_viewChoices->setChosen(stage);

	int levelupCost = 0;
	int levelupTotalInvestment = 0;

	if (m_spendingPoints)
	{
		m_gameIconList->registerDongle("spend_exp_x");
		m_gameIconList->setDongleOffset("spend_exp_x", sf::Vector2i(-10, -8));

		m_gameIconList->registerDongle("spend_exp_min");
		m_gameIconList->setDongleOffset("spend_exp_min", sf::Vector2i(30, -8));

		for (auto& itr : getDesiredInvestments())
			levelupTotalInvestment += itr.second;

		if (auto myself = world().myself())
		{
			levelupCost = PlayerFunctions::computeSpellUpgradeCost(myself->getVariable(ObjDefines::Variable::NumInvestedSpells) + levelupTotalInvestment);
			levelupTotalInvestment += myself->getVariable(ObjDefines::Variable::NumInvestedSpells);
		}
	}

	for (auto& itr : m_spells[stage])
	{
		m_gameIconList->addSpellIcon(itr.spellId, itr.level + m_desiredInvestments[itr.spellId], itr.bpoints);

		if (m_spendingPoints && SpellIcon::canSpellScale(itr.spellId))
		{
			auto tt = Equipment::spawnUpgradeTooltip(levelupTotalInvestment, levelupCost, "Talent", "talent");
			m_gameIconList->registerDongleTooltip("spend_exp_x", itr.spellId, tt);
			m_gameIconList->registerDongleTooltip("spend_exp_min", itr.spellId, tt);
		}
	}
}
		
void Abilities::updateData(const int spellId, const uint8_t level, const vector<pair<int16_t, int16_t>>& bpoints)
{
	for (auto& itr : m_spells)
	{
		for (auto& subItr : itr.second)
		{
			if (subItr.spellId == spellId)
			{
				int lvl = level;

				// Prioritize client value for this
				if (!m_spendingPoints)
					subItr.level = level;
				else
					lvl = subItr.level + m_desiredInvestments[subItr.spellId];

				if (lvl != level && m_spendingPoints)
					requestTheorySpell(subItr.spellId, lvl);
				
				subItr.bpoints = bpoints;
				m_gameIconList->updateSpellIcon(subItr.spellId, lvl, subItr.bpoints);
			}
		}
	}

	refreshTooltips();
}

int Abilities::spellSlotCount(int stage) const
{
	auto itr = m_spells.find(static_cast<Stage>(stage));
	return itr == m_spells.end() ? 0 : static_cast<int>(itr->second.size());
}

bool Abilities::spellSlotAt(int stage, int index, int& spellId, int& level) const
{
	auto itr = m_spells.find(static_cast<Stage>(stage));
	if (itr == m_spells.end() || index < 0 || index >= static_cast<int>(itr->second.size()))
		return false;
	spellId = itr->second[index].spellId;
	level   = itr->second[index].level;
	return true;
}

void Abilities::setData(const vector<GP_Server_Spellbook::SpellSlot>& spells)
{
	m_spells.clear();

	auto& sv = sContentMgr->db("spell_template");

	for (auto& itr : spells)
	{
		int abilities_tab = atoi(sv.data(itr.spellId, "abilities_tab").c_str());
		m_spells[Stage(abilities_tab)].push_back(itr);
	}

	setStage(m_stage);
	refreshTooltips();
}

void Abilities::refreshTooltips()
{
	m_gameIconList->refreshTooltips();
}
		
bool Abilities::hasSpell(const int spellId) const
{
	for (auto& itr : m_spells)
	{
		for (auto& subItr: itr.second)
		{
			if (subItr.spellId == spellId)
				return true;
		}
	}

	return false;
}

bool Abilities::getSpellPoints(const int spellId, GP_Server_Spellbook::SpellSlot& output) const
{
	for (auto& itr : m_spells)
	{
		for (auto& subItr: itr.second)
		{
			if (subItr.spellId == spellId)
			{
				output = subItr;
				return true;
			}
		}
	}

	return false;
}