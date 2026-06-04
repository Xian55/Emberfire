#include "stdafx.h"
#include "Equipment.h"
#include "ItemIcon.h"
#include "ClientPlayer.h"
#include "TextBoxRo.h"
#include "World.h"
#include "Inventory.h"
#include "Connector.h"
#include "SelectionButtons.h"
#include "ContentMgr.h"
#include "Tooltip.h"
#include "Application.h"
#include "Text.h"
#include "Abilities.h"
#include "Sprite.h"

#include "..\SqlConnector\QueryResult.h"
#include "..\StringHelpers.h"
#include "..\Shared\GamePacketClient.h"

#include <assert.h>

Equipment::Equipment(World& owner, const int id) :
	WorldPanel(owner, id, "equipment.png", sf::Vector2i(541, 28))
{
	// Name
	addRenderObject(attachObj(make_shared<TextBoxRo>(*this, Interface::NameLabel, "Palatino Linotype Regular.ttf", 202, 15, TextBox::AlignCenterAbsolute), { 188, 121 }));
	addRenderObject(attachObj(make_shared<TextBoxRo>(*this, Interface::SubnameLabel, "Palatino Linotype Regular.ttf", 202, 12, TextBox::AlignCenterAbsolute), { 188, 141 }));
	
	// Inventory
	addRenderObject(attachObj(m_levelupButton = make_shared<Button>(*this, "level_up", Interface::LevelUp), sf::Vector2i(14, 527)));
	addRenderObject(attachObj(make_shared<Button>(*this, "level_up_confirm", Interface::ConfirmLevelUp), sf::Vector2i(14, 527)));
	addRenderObject(attachObj(make_shared<Button>(*this, "level_up_cancel", Interface::CancelLevelUp), sf::Vector2i(147, 527)));
	addRenderObject(attachObj(m_levelupPurpleButton = make_shared<Button>(*this, "spend_xp_notify", Interface::LevelUpExclaim), sf::Vector2i(-5, 502)));
	getRenderObject(Interface::ConfirmLevelUp)->setHidden(true);
	getRenderObject(Interface::CancelLevelUp)->setHidden(true);
	getRenderObject(Interface::LevelUpExclaim)->setHidden(true);

	// Left side
	addRenderObject(attachObj(configSlot(make_shared<ItemIcon>(*this, Interface::HelmIcon, "gameicon40")), { 46,178 }));
	addRenderObject(attachObj(configSlot(make_shared<ItemIcon>(*this, Interface::NecklaceIcon, "gameicon40")), { 46,178 + (58 * 1) }));
	addRenderObject(attachObj(configSlot(make_shared<ItemIcon>(*this, Interface::ChestIcon, "gameicon40")), { 46,178 + (58 * 2) }));
	addRenderObject(attachObj(configSlot(make_shared<ItemIcon>(*this, Interface::RangedIcon, "gameicon40")), { 46,178 + (58 * 3) }));
	addRenderObject(attachObj(configSlot(make_shared<ItemIcon>(*this, Interface::ShieldIcon, "gameicon40")), { 46,178 + (58 * 4) }));
	addRenderObject(attachObj(configSlot(make_shared<ItemIcon>(*this, Interface::WeaponIcon, "gameicon40")), { 46,178 + (58 * 5) }));

	// Right side
	addRenderObject(attachObj(configSlot(make_shared<ItemIcon>(*this, Interface::Ring1Icon, "gameicon40")), { 289,178 }));
	addRenderObject(attachObj(configSlot(make_shared<ItemIcon>(*this, Interface::Ring2Icon, "gameicon40")), { 289,178 + (58 * 1) }));
	addRenderObject(attachObj(configSlot(make_shared<ItemIcon>(*this, Interface::GlovesIcon, "gameicon40")), { 289,178 + (58 * 2) }));
	addRenderObject(attachObj(configSlot(make_shared<ItemIcon>(*this, Interface::BeltIcon, "gameicon40")), { 289,178 + (58 * 3) }));
	addRenderObject(attachObj(configSlot(make_shared<ItemIcon>(*this, Interface::LegsIcon, "gameicon40")), { 289,178 + (58 * 4) }));
	addRenderObject(attachObj(configSlot(make_shared<ItemIcon>(*this, Interface::FeetIcon, "gameicon40")), { 289,178 + (58 * 5) }));
	
	// Changing view optionss
	m_viewChoices = make_shared<SelectionButtons>(*this, Interface::StatsTabs);
	m_tab1 = make_shared<Button>(*m_viewChoices, "equipment_view", SView_General);
	m_tab2 = make_shared<Button>(*m_viewChoices, "equipment_view", SView_Combat);
	m_tab3 = make_shared<Button>(*m_viewChoices, "equipment_view", SView_Skills);
	m_viewChoices->addRenderObject(attachObj(m_tab1, { 110, 70 }));
	m_viewChoices->addRenderObject(attachObj(m_tab2, { 250, 70 }));
	m_viewChoices->addRenderObject(attachObj(m_tab3, { 384, 70 }));
	addRenderObject(m_viewChoices);

	m_labelColor = sf::Color(95, 82, 72, 255);
	m_labelShadow = sf::Color(0, 0, 0, 38);
	m_levelupCostLabel = sContentMgr->spawnSprite("levelup_cost_label.png");
	m_levelupBaseNotice = sContentMgr->spawnSprite("equipment_baseinvest_msg.png");
	m_levelupBaseNotice->setHotspotEasy(true, true);

	m_crLabel = sContentMgr->spawnSprite("combat_rating_label.png");

	setStatView(SView_General);

	//debugging
	m_player = make_shared<ClientPlayer>(0, nullptr, PlayerDefines::Classes::Paladin, PlayerDefines::Gender::Male, 1);
} 

Equipment::~Equipment()
{

}

void Equipment::input()
{
	__super::input();
	__super::grabIcon();

	if (popButtonId(Interface::LevelUp) || popButtonId(Interface::LevelUpExclaim))
	{
		if (m_wasOpenedRecently)
			sContentMgr->playSound("chun_command_center_edit.ogg");
		else
			sContentMgr->playSound("button_click_a.ogg");

		m_wasOpenedRecently = false;
		toggleSpendingPoints(true);
	}

	if (popButtonId(Interface::CancelLevelUp))
	{
		toggleSpendingPoints(false);
		sConnector->sendPacket(GP_Client_ReqAbilityList{}.build(StlBuffer{}));
	}

	if (popButtonId(Interface::ConfirmLevelUp))
	{
		if (!m_pendingServerSpend)
			world().requestLevelup();

		m_pendingServerSpend = true;
	}

	if (m_viewChoices->getChosen() != m_view)
		setStatView(StatView(m_viewChoices->getChosen()));
	
	if (!m_spendingPoints)
	{
		m_levelupButton->setHidden(m_levelupButton->isExclaimNotice());
		m_levelupPurpleButton->setHidden(!m_levelupButton->isExclaimNotice());
	}

	// +
	for (auto& id : m_currentAddBtns)
	{
		if (id.second->popActivated())
		{
			// Spending points into leveling up
			int var = id.first - Interface::SpendPointKey;
			UnitDefines::Stat stat = UnitDefines::Stat(var - ObjDefines::Variable::StatsStart);
			m_pendingStatInvestments[stat] += 1;

			calculateBaseStats();
			world().computePendingLevelupCost();
			refreshPlusMinusStatSpending_Slow();
		}
	}

	// -
	for (auto& id : m_currentMinusBtns)
	{
		if (id.second->popActivated())
		{
			int var = id.first - Interface::SpendPointMinusKey;
			UnitDefines::Stat stat = UnitDefines::Stat(var - ObjDefines::Variable::StatsStart);

			if (m_pendingStatInvestments[stat] > 0)
				m_pendingStatInvestments[stat] -= 1;

			calculateBaseStats();
			world().computePendingLevelupCost();
			refreshPlusMinusStatSpending_Slow();
		}
	}
}

void Equipment::render()
{
	// The player's stats might change at any time
	if (m_player != nullptr)
		updatePlayer(m_player);

	if (!isHidden())
	{
		// Opaque backing — the panel art (equipment.png) is semi-transparent, so the stat sheet bleeds the
		// game world through and is hard to read. Draw a solid dark fill behind it first.
		const sf::Vector2f tl(getTopLeftCornerRef());
		sf::RectangleShape backing(sf::Vector2f(getBottomRightCornerRef()) - tl);
		backing.setPosition(tl);
		backing.setFillColor(sf::Color(18, 14, 10, 250));
		sApplication->canvas().draw(backing);
	}

	__super::render();

	for (auto& itr : m_currentLabels)
	{
		// Description of the stat on mouseover
		if (itr.second->isMousedOver())
		{
			itr.second->getTextRef().getTextRef().setFillColor(sf::Color::Yellow);

			int var = itr.first - Interface::LabelsKey;
			
			if (var >= ObjDefines::Variable::StatsStart && var <= ObjDefines::Variable::StatsEnd)
			{
				UnitDefines::Stat stat = UnitDefines::Stat(var - ObjDefines::Variable::StatsStart);

				if (auto ptr = sContentMgr->getStatTooltip(m_player->getClass(), stat))
				{
					// Position to the left of the label
					sf::Vector2i topLeft(itr.second->getTextRef().getTextRef().getPosition());
					topLeft.x -= ptr->getWidth() + 53;
					ptr->moveTo(topLeft);

					sApplication->setTooltip(ptr);
				}
			}
		}
		else
		{
			itr.second->getTextRef().getTextRef().setFillColor(m_labelColor);
		}
	}

	if (m_spendingPoints)
	{
		sf::Vector2f levelupCostLabelPos = sf::Vector2f(getTopLeftCornerRef()) + sf::Vector2f(171.f, 335.f);
		m_levelupCostLabel->render(levelupCostLabelPos);

		if (m_view == StatView::SView_General || m_view == StatView::SView_Skills)
			m_levelupBaseNotice->render(sf::Vector2f(getTopLeftCornerRef()) + sf::Vector2f(460.f, 460.f));
		else
			m_levelupBaseNotice->render(sf::Vector2f(getTopLeftCornerRef()) + sf::Vector2f(190.f, 460.f));

		if (m_lastKnownLevelupCost != world().getCachedPendingLevelupCost() && !sf::Mouse::isButtonPressed(sf::Mouse::Left))
		{
			int diff = world().getCachedPendingLevelupCost() - m_lastKnownLevelupCost;
			string msg = (diff > 0 ? "+" : "-") + to_string(abs(diff));

			world().flushCombatMsgs(0);
			world().pushCombatMessage(msg, sf::Vector2i(sf::Vector2f(levelupCostLabelPos.x + m_levelupCostLabel->getGlobalBounds().width + 5.f, levelupCostLabelPos.y + 15.f)), sf::Color(141, 0, 135, 255), false, 0.7f, 0, false, 1.f, false, true, false, true);
		}

		m_lastKnownLevelupCost = world().getCachedPendingLevelupCost();
	}

	if (!m_spendingPoints)
	{
		if (m_world.myself() != nullptr && m_world.myself()->getLevel() >= Defines::MaxPlayerLevel)
		{
			sf::Vector2f crLabelPos = sf::Vector2f(getTopLeftCornerRef()) + sf::Vector2f(135.f, 335.f);
			m_crLabel->render(crLabelPos);

			attachValue(CustomLabel::CombatRating, { 191, 358 }, TextBox::AlignCenterAbsolute, false);
			attachValue(CustomLabel::CombatRatingText, { 191, 375 }, TextBox::AlignCenterAbsolute, false);
		}
	}

	dynamic_pointer_cast<Button>(getRenderObject(Interface::ConfirmLevelUp))->setExclaimNotice(m_spendingPoints && m_world.getCachedPendingLevelupCost() != 0);

	if (!wasInputLastFrame())
		m_wasOpenedRecently = true;
}
	
void Equipment::onClose() /*final*/
{
	sConnector->sendPacket(GP_Client_ReqAbilityList{}.build(StlBuffer{}));

	// Re-notify if they failed to do the deed
	if (dynamic_pointer_cast<Button>(getRenderObject(Equipment::Interface::LevelUp))->isExclaimNotice())
		m_world.exclaimHint(World::Hint::SpendExp);
}
		
shared_ptr<ItemIcon>& Equipment::configSlot(shared_ptr<ItemIcon>& ptr)
{
	ptr->setEnableDragActivate(true);
	ptr->setShowSellPrice(false);
	return ptr;
}

bool Equipment::isAllow_AddStat(const UnitDefines::Stat stat) const
{
	if (!m_spendingPoints)
		return false;

	if (!canUseSkill(stat))
		return false;

	if (PlayerFunctions::computeStatUpgradeCost(stat, 0) < 0)
		return false;

	int pendingBonus = 0;

	auto itr = m_pendingStatInvestments.find(stat);

	if (itr != m_pendingStatInvestments.end())
		pendingBonus = itr->second;

	int cap = PlayerFunctions::getStatUpgradeCap(stat);

	if (m_player->getBaseStat(stat) + m_player->getBaseStatBonus(stat) + pendingBonus >= cap)
		return false;
	
	return true;
}

bool Equipment::isAllow_MinusStat(const UnitDefines::Stat stat) const
{
	if (!m_spendingPoints)
		return false;

	auto itr = m_pendingStatInvestments.find(stat);

	if (itr == m_pendingStatInvestments.end())
		return false;

	return itr->second > 0;
}
		
void Equipment::toggleSpendingPoints(const bool v)
{
	if (m_spendingPoints == v)
		return;
	
	getRenderObject(Interface::LevelUp)->setHidden(v);
	getRenderObject(Interface::LevelUpExclaim)->setHidden(v);
	getRenderObject(Interface::ConfirmLevelUp)->setHidden(!v);
	getRenderObject(Interface::CancelLevelUp)->setHidden(!v);
	dynamic_pointer_cast<Abilities>(world().getRenderObject(World::Interface::AbilitiesPanel))->toggleSpendingPoints(v);

	m_pendingStatInvestments.clear();
	m_spendingPoints = v;	
	m_pendingServerSpend = false;
	m_lastKnownLevelupCost = 0;

	if (v)
		setStatView(StatView::SView_General);
	else
		setStatView(m_view);

	if (v)
	{
		calculateBaseStats();
		world().closePanels({ World::Interface::InventoryPanel }, false);
		world().openPanel(World::Interface::AbilitiesPanel, false);
		world().openPanel(World::Interface::EquipmentPanel, false);
		world().computePendingLevelupCost();
		dynamic_pointer_cast<Abilities>(world().getRenderObject(World::Interface::AbilitiesPanel))->setStage(Abilities::Stage::Spellbook);

		if (sApplication->getTutorialStatus("StatSpending") == false)
		{
			m_tab2->setExclaimNotice(true);
			m_tab3->setExclaimNotice(true);
			sApplication->setTutorialStatus("StatSpending", true);
		}
	}
	else
	{
		m_tab2->setExclaimNotice(false);
		m_tab3->setExclaimNotice(false);
		world().refreshToolbarTooltips();
	}
}

/*static*/
shared_ptr<Tooltip> Equipment::spawnUpgradeTooltip(const int totalInvest, const int cost, const string& itemName, const string& pointName)
{
	shared_ptr<Tooltip> tt = make_shared<Tooltip>(*sApplication, sf::Vector2i());
	tt->setShadowOffset(1);
	tt->setOffset(sf::Vector2i(65, -35));
	tt->addLine("trebuc.ttf", 15, "+1 " + itemName + ": ", sf::Color(240, 197, 2, 255), false);
	tt->addLine("trebuc.ttf", 15, Util::formatMoneyCommas(cost) + "xp", sf::Color(181, 0, 175, 255));

	if (totalInvest > 1)
		tt->addLine("trebuc.ttf", 15, "You have invested " + to_string(totalInvest) + " " + pointName + "s.");
	else if (totalInvest > 0)
		tt->addLine("trebuc.ttf", 15, "You have invested " + to_string(totalInvest) + " " + pointName + ".");
	else
		tt->addLine("trebuc.ttf", 15, "You haven't invested anything.");

	return tt;
}

void Equipment::refreshPlusMinusStatSpending_Slow()
{
	map<UnitDefines::Stat, shared_ptr<Tooltip>> tooltips;

	for (auto& id : m_currentAddBtns)
	{
		int var = id.first - Interface::SpendPointKey;
		UnitDefines::Stat stat = UnitDefines::Stat(var - ObjDefines::Variable::StatsStart);
		id.second->setHidden(!isAllow_AddStat(stat));

		if (!id.second->isHidden() || isAllow_MinusStat(stat))
		{
			int totalInvest = m_player->getBaseStat(stat) + m_pendingStatInvestments[stat];
			shared_ptr<Tooltip> tt = spawnUpgradeTooltip(totalInvest, PlayerFunctions::computeStatUpgradeCost(stat, totalInvest), UnitFunctions::statName(stat), "point");			
			tt->setAnchor(&id.second->getTopLeftCornerRef());
			id.second->setTooltip(tt);
			tooltips[stat] = tt;
		}
	}

	for (auto& id : m_currentMinusBtns)
	{
		int var = id.first - Interface::SpendPointMinusKey;
		UnitDefines::Stat stat = UnitDefines::Stat(var - ObjDefines::Variable::StatsStart);
		id.second->setHidden(!isAllow_MinusStat(stat));

		if (!id.second->isHidden())
			id.second->setTooltip(tooltips[stat]);
	}
}

void Equipment::setStatView(const StatView view)
{
	m_view = view;
	m_viewChoices->setChosen(view);
	m_viewChoices->popChange();

	for (auto& id : m_currentLabels)
		destroyObjectById(id.first);

	for (auto& id : m_currentValues)
		destroyObjectById(id.first);

	for (auto& id : m_currentAddBtns)
		destroyObjectById(id.first);

	for (auto& id : m_currentMinusBtns)
		destroyObjectById(id.first);
	
	m_currentLabels.clear();
	m_currentValues.clear();
	m_currentAddBtns.clear();
	m_currentMinusBtns.clear();
	
	int valuesX = 500;
	int labelsX = 380;

	if (m_spendingPoints)
	{
		valuesX = 490;
		labelsX = 376;
	}

	if (view == StatView::SView_General)
	{
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Health, { labelsX, 140 });  // label "Health"; value reads MaxHealth below
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Mana, { labelsX, 140 + (25 * 1)});
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::ArmorValue, { labelsX, 140 + (25 * 2) });

		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Strength, { labelsX, 140 + (25 * 4) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Agility, { labelsX, 140 + (25 * 5) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Willpower, { labelsX, 140 + (25 * 6) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Intelligence, { labelsX, 140 + (25 * 7) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Courage, { labelsX, 140 + (25 * 8) });
		
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Regeneration, { labelsX, 140 + (25 * 10) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Meditate, { labelsX, 140 + (25 * 11) });

		if (!m_spendingPoints)
		{
			attachLabel(ObjDefines::Variable::PkCount, { labelsX, 140 + (25 * 13) });
			attachLabel(ObjDefines::Variable::ArenaRating, { labelsX, 140 + (25 * 14) });
			attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Fortitude, { labelsX, 140 + (25 * 15) });
		}

		//..

		attachValue(ObjDefines::Variable::MaxHealth, { valuesX, 140 }, TextBox::AlignLeft, true,
			ObjDefines::Variable::StatsStart + UnitDefines::Stat::Health);  // display MaxHealth, invest Stat::Health
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Mana, { valuesX, 140 + (25 * 1)}, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::ArmorValue, { valuesX, 140 + (25 * 2) }, TextBox::AlignLeft);

		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Strength, { valuesX, 140 + (25 * 4) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Agility, { valuesX, 140 + (25 * 5) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Willpower, { valuesX, 140 + (25 * 6) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Intelligence, { valuesX, 140 + (25 * 7) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Courage, { valuesX, 140 + (25 * 8) }, TextBox::AlignLeft);
		
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Regeneration, { valuesX, 140 + (25 * 10) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Meditate, { valuesX, 140 + (25 * 11) }, TextBox::AlignLeft);

		if (!m_spendingPoints)
		{
			attachValue(ObjDefines::Variable::PkCount, { valuesX, 140 + (25 * 13) }, TextBox::AlignLeft);
			attachValue(ObjDefines::Variable::ArenaRating, { valuesX, 140 + (25 * 14) }, TextBox::AlignLeft);
			attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Fortitude, { valuesX, 140 + (25 * 15) }, TextBox::AlignLeft);
		}		
	}
	else if (view == StatView::SView_Combat)
	{
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::MeleeValue, { labelsX, 140 });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::MeleeCooldown, { labelsX, 140 + (25 * 1) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::RangedValue, { labelsX, 140 + (25 * 2) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::RangedCooldown, { labelsX, 140 + (25 * 3) });

		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::MeleeCritical, { labelsX, 140 + (25 * 5) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::RangedCritical, { labelsX, 140 + (25 * 6) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::SpellCritical, { labelsX, 140 + (25 * 7) });
		
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::DodgeRating, { labelsX, 140 + (25 * 9) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::BlockRating, { labelsX, 140 + (25 * 10) });

		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::ResistFrost, { labelsX, 140 + (25 * 12) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::ResistFire, { labelsX, 140 + (25 * 13) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::ResistShadow, { labelsX, 140 + (25 * 14) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::ResistHoly, { labelsX, 140 + (25 * 15) });

		//..

		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::MeleeValue, { valuesX, 140 }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::MeleeCooldown, { valuesX, 140 + (25 * 1) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::RangedValue, { valuesX, 140 + (25 * 2) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::RangedCooldown, { valuesX, 140 + (25 * 3) }, TextBox::AlignLeft);

		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::MeleeCritical, { valuesX, 140 + (25 * 5) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::RangedCritical, { valuesX, 140 + (25 * 6) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::SpellCritical, { valuesX, 140 + (25 * 7) }, TextBox::AlignLeft);
		
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::DodgeRating, { valuesX, 140 + (25 * 9) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::BlockRating, { valuesX, 140 + (25 * 10) }, TextBox::AlignLeft);

		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::ResistFrost, { valuesX, 140 + (25 * 12) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::ResistFire, { valuesX, 140 + (25 * 13) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::ResistShadow, { valuesX, 140 + (25 * 14) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::ResistHoly, { valuesX, 140 + (25 * 15) }, TextBox::AlignLeft);
	}
	else if (view == StatView::SView_Skills)
	{
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::StaffSkill, { labelsX, 140 + (25 * 0) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::MaceSkill, { labelsX, 140 + (25 * 1) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::AxesSkill, { labelsX, 140 + (25 * 2) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::SwordSkill, { labelsX, 140 + (25 * 3) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::RangedSkill, { labelsX, 140 + (25 * 4) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::DaggerSkill, { labelsX, 140 + (25 * 5) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::WandSkill, { labelsX, 140 + (25 * 6) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::ShieldSkill, { labelsX, 140 + (25 * 7) });

		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Bartering, { labelsX, 140+ (25 * 9) });
		attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Lockpicking, { labelsX, 140 + (25 * 10) });
		//attachLabel(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Perception, { labelsX, 140 + (25 * 11) });

		//..

		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::StaffSkill, { valuesX, 140 + (25 * 0) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::MaceSkill, { valuesX, 140 + (25 * 1) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::AxesSkill, { valuesX, 140 + (25 * 2) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::SwordSkill, { valuesX, 140 + (25 * 3) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::RangedSkill, { valuesX, 140 + (25 * 4) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::DaggerSkill, { valuesX, 140 + (25 * 5) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::WandSkill, { valuesX, 140 + (25 * 6) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::ShieldSkill, { valuesX, 140 + (25 * 7) }, TextBox::AlignLeft);

		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Bartering, { valuesX, 140+ (25 * 9) }, TextBox::AlignLeft);
		attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Lockpicking, { valuesX, 140 + (25 * 10) }, TextBox::AlignLeft);
		//attachValue(ObjDefines::Variable::StatsStart + UnitDefines::Stat::Perception, { valuesX, 140 + (25 * 11) }, TextBox::AlignLeft);
	}
		
	attachValue(ObjDefines::Variable::Progression, { 191, 248 }, TextBox::AlignCenterAbsolute, false);
	attachValue(ObjDefines::Variable::Experience, { 191, 303 }, TextBox::AlignCenterAbsolute, false);

	if (m_spendingPoints)
	{
		attachValue(CustomLabel::LevelupCost, { 191, 358 }, TextBox::AlignCenterAbsolute, false);
	}

	refreshPlusMinusStatSpending_Slow();
}

shared_ptr<TextBoxRo> Equipment::attachLabel(const int var, const sf::Vector2i& offset)
{
	auto textRo = make_shared<TextBoxRo>(*this, Interface::LabelsKey + var, "Palatino Linotype Regular.ttf", 500, 14, TextBox::AlignLeft, true, 2.f);

	string labelStr;

	if (var == ObjDefines::Variable::ArenaRating)
	{
		labelStr = "Arena Rating";
		textRo->setMouseable(false);
	}
	else if (var == ObjDefines::Variable::PkCount)
	{
		labelStr = "Player Kills";
		textRo->setMouseable(false);
	}
	else
	{
		labelStr = UnitFunctions::statName(UnitDefines::Stat(var - ObjDefines::Variable::StatsStart)) + ":";
		textRo->setMouseable(true);
	}

	textRo->setAnchor(&getTopLeftCornerRef());
	textRo->setOffset(offset);
	textRo->setString(labelStr, m_labelColor, m_labelShadow);
	addRenderObject(textRo);

	return m_currentLabels[textRo->getId()] = textRo;
}

shared_ptr<TextBoxRo> Equipment::attachValue(const int var, const sf::Vector2i& offset, const int alignment, const bool spendButtons /*= true*/, const int spendVar /*= -1*/)
{
	auto textRo = make_shared<TextBoxRo>(*this, Interface::ValuesKey + var, "Palatino Linotype Regular.ttf", 500, 16, TextBox::Align(alignment), true, 2.f);
	textRo->setAnchor(&getTopLeftCornerRef());
	textRo->setOffset(offset);
	addRenderObject(textRo);
	m_currentValues[textRo->getId()] = textRo;

	if (spendButtons)
	{
		// The value displays `var`, but the spend buttons can invest a DIFFERENT stat: the sheet Health row
		// shows MaxHealth (var 0x03) yet must invest Stat::Health (var 0x0e). spendVar<0 -> same as var.
		const int sVar = (spendVar < 0) ? var : spendVar;
		attachSpendButton(sVar, offset + sf::Vector2i(35, -7));
		attachSpendMinButton(sVar, offset + sf::Vector2i(55, -7));
	}

	return textRo;
}

shared_ptr<Button> Equipment::attachSpendMinButton(const int var, const sf::Vector2i& offset)
{
	auto button = make_shared<Button>(*this, "spend_exp_min", Interface::SpendPointMinusKey + var);
	button->setAnchor(&getTopLeftCornerRef());
	button->setOffset(offset);
	button->setEnableLeftFire(true);
	addRenderObject(button);
	return m_currentMinusBtns[button->getId()] = button;
}

shared_ptr<Button> Equipment::attachSpendButton(const int var, const sf::Vector2i& offset)
{
	auto button = make_shared<Button>(*this, "spend_exp_x", Interface::SpendPointKey + var);
	button->setAnchor(&getTopLeftCornerRef());
	button->setOffset(offset);
	button->setEnableLeftFire(true);
	addRenderObject(button);
	return m_currentAddBtns[button->getId()] = button;
}

bool Equipment::overrideIconGrab(shared_ptr<GameIcon> targetIcon) /*final*/
{
	if (world().getItemAction() != nullptr)
	{
		GP_Client_UseItem packet;
		packet.m_slot = world().getItemAction()->getId();
		packet.m_itemId = world().getItemAction()->getEntry();
		packet.m_target_EquipSlot = Equipment::convertInterface(static_cast<Equipment::Interface>(targetIcon->getId()));
		sConnector->sendPacket(packet.build(StlBuffer{}));

		// Clear
		world().setItemAction(nullptr);
		return true;
	}

	return __super::overrideIconGrab(targetIcon);
}
		
string Equipment::getEquipmentValue(const int var)
{
	if (var >= ObjDefines::Variable::StatsStart && var <= ObjDefines::Variable::StatsEnd)
	{
		UnitDefines::Stat stat = UnitDefines::Stat(var - ObjDefines::Variable::StatsStart);

		switch (stat)
		{
			case UnitDefines::Stat::MeleeCooldown:
			{
				return Util::fmtStr("%.2f", static_cast<float>(m_player->getStat(UnitDefines::Stat::MeleeCooldown)) / 1000.f);
			}
			case UnitDefines::Stat::RangedCooldown:
			{
				return Util::fmtStr("%.2f", static_cast<float>(m_player->getStat(UnitDefines::Stat::RangedCooldown)) / 1000.f);
			}
			default:
			{
				if (m_spendingPoints && (isAllow_AddStat(stat) || isAllow_MinusStat(stat)))
					return to_string(m_cachedBaseStats[stat]);

				return to_string(m_player->getStat(stat));
			}
		}
	}

	switch (var)
	{
		case CustomLabel::LevelupCost:
		{
			return Util::formatMoneyCommas(world().getCachedPendingLevelupCost());
		}
		case CustomLabel::CombatRating:
		{
			if (auto myself = m_world.myself())
			{
				int cr = myself->getVariable(ObjDefines::Variable::CombatRating);
				int crMax = myself->getVariable(ObjDefines::Variable::CombatRatingMax);

				if (crMax < 1)
					crMax = 1;

				float ratio = float(cr) / float(crMax);
				int displayCr = int(ratio * 10000);
				int displayMax = int(10000);

				return Util::fmtStr("%s / %s", Util::formatMoneyCommas(displayCr).c_str(), Util::formatMoneyCommas(displayMax).c_str());
			}

			return "Unknown";
		}
		case CustomLabel::CombatRatingText:
		{
			if (auto myself = m_world.myself())
			{
				int cr = myself->getVariable(ObjDefines::Variable::CombatRating);
				int crMax = myself->getVariable(ObjDefines::Variable::CombatRatingMax);

				if (crMax < 1)
					crMax = 1;

				float ratio = float(cr) / float(crMax);

				std::string scoreStr;

				if (crMax > 0)
				{
					if (ratio >= 0.95f)
						scoreStr = "Overgod";
					else if (ratio >= 0.85f)
						scoreStr = "Godly";
					else if (ratio >= 0.7f)
						scoreStr = "Demigod";
					else if (ratio >= 0.6f)
						scoreStr = "Divine";
					else if (ratio > 0.5f)
						scoreStr = "Hero";
					else if (ratio > 0.4f)
						scoreStr = "Renowned";
					else if (ratio > 0.15f)
						scoreStr = "Mortal";
					else
						scoreStr = "Feeble";
				}
				else
				{
					scoreStr = "Unknown";
				}

				return "(" + scoreStr + ")";
			}

			return "Unknown";
		}
		case ObjDefines::Variable::Progression:
		{
			auto& sv = sContentMgr->db("player_exp_levels");
			int requiredAmount = atoi(sv.data(m_player->getLevel() + 1, "exp").c_str());

			if (requiredAmount > 0)
				return Util::fmtStr("%d / %d", m_player->getVariable(ObjDefines::Variable(var)), requiredAmount);

			break;
		}
		case ObjDefines::Variable::Experience:
		{
			return Util::formatMoneyCommas(m_player->getVariable(ObjDefines::Variable(var)));
		}
	}

	return to_string(m_player->getVariable(ObjDefines::Variable(var)));
}

bool Equipment::canUseSkill(const UnitDefines::Stat stat) const
{
	switch (stat)
	{
		case UnitDefines::Stat::StaffSkill:				
			return PlayerFunctions::canEquipItem(m_player->getClass(), ItemDefines::WeaponType::Staff);
		case UnitDefines::Stat::MaceSkill:
			return PlayerFunctions::canEquipItem(m_player->getClass(), ItemDefines::WeaponType::Mace);
		case UnitDefines::Stat::AxesSkill:
			return PlayerFunctions::canEquipItem(m_player->getClass(), ItemDefines::WeaponType::Axe);
		case UnitDefines::Stat::SwordSkill:
			return PlayerFunctions::canEquipItem(m_player->getClass(), ItemDefines::WeaponType::Sword);
		case UnitDefines::Stat::RangedSkill:
			return PlayerFunctions::canEquipItem(m_player->getClass(), ItemDefines::WeaponType::Bow);
		case UnitDefines::Stat::DaggerSkill:
			return PlayerFunctions::canEquipItem(m_player->getClass(), ItemDefines::WeaponType::Dagger);
		case UnitDefines::Stat::WandSkill:
			return PlayerFunctions::canEquipItem(m_player->getClass(), ItemDefines::WeaponType::Wand);
		case UnitDefines::Stat::ShieldSkill:
			return PlayerFunctions::canEquipItem(m_player->getClass(), ItemDefines::EquipType::Shield);
	}

	return true;
}

sf::Color Equipment::variableVaueColor(const int var)
{
	// Sheet Health value reads MaxHealth (var 0x03); colour it like the Health stat (red).
	if (var == ObjDefines::Variable::MaxHealth)
		return sf::Color(156, 74, 51, 255);

	if (var >= ObjDefines::Variable::StatsStart && var <= ObjDefines::Variable::StatsEnd)
	{
		UnitDefines::Stat stat = UnitDefines::Stat(var - ObjDefines::Variable::StatsStart);

		switch (stat)
		{
			case UnitDefines::Stat::Health: return sf::Color(156, 74, 51, 255);
			case UnitDefines::Stat::Mana: return sf::Color(55, 111, 184, 255);
			case UnitDefines::Stat::ResistFrost: return sf::Color(55, 182, 184, 255);
			case UnitDefines::Stat::ResistFire: return sf::Color(188, 104, 48, 255);
			case UnitDefines::Stat::ResistShadow: return sf::Color(61, 48, 188, 255);
			case UnitDefines::Stat::ResistHoly: return sf::Color(188, 48, 147, 255);	
		}

		switch (stat)
		{
			case UnitDefines::Stat::StaffSkill:
			case UnitDefines::Stat::MaceSkill:
			case UnitDefines::Stat::AxesSkill:
			case UnitDefines::Stat::SwordSkill:
			case UnitDefines::Stat::RangedSkill:
			case UnitDefines::Stat::DaggerSkill:
			case UnitDefines::Stat::WandSkill:
			case UnitDefines::Stat::ShieldSkill:
			{
				if (!canUseSkill(stat))
					return sf::Color(0x9c4a33FF);
				
				return sf::Color(0x88bd68FF);
			}
		}
	}

	return sf::Color(148, 133, 116, 255);
}

void Equipment::calculateBaseStats()
{
	m_cachedBaseStats.clear();

	if (m_spendingPoints)
	{
		for (int i = 0; i < UnitDefines::Stat::NumStats; ++i)
		{
			UnitDefines::Stat stat = decltype(stat)(i);
			m_cachedBaseStats[stat] = m_player->getBaseStat(stat) + m_player->getBaseStatBonus(stat) + m_pendingStatInvestments[stat];
		}

		for (auto& itr : m_cachedBaseStats)
			m_player->applyStatModifierLogic(m_player->getClass(), itr.first, itr.second, m_cachedBaseStats);
	}
}

void Equipment::updatePlayer(shared_ptr<ClientPlayer> plr)
{
	m_player = plr;

	for (auto& id : m_currentValues)
	{
		const int var = id.first - Interface::ValuesKey;
		id.second->setString(getEquipmentValue(var), variableVaueColor(var), sf::Color(0, 0, 0, 38));
	}
		
	// Gear
	dynamic_pointer_cast<ItemIcon>(getRenderObject(Interface::HelmIcon))->setItemDef(plr->getEquipmentItemId(UnitDefines::Helm));
	dynamic_pointer_cast<ItemIcon>(getRenderObject(Interface::WeaponIcon))->setItemDef(plr->getEquipmentItemId(UnitDefines::Weapon1));
	dynamic_pointer_cast<ItemIcon>(getRenderObject(Interface::ShieldIcon))->setItemDef(plr->getEquipmentItemId(UnitDefines::Offhand));
	dynamic_pointer_cast<ItemIcon>(getRenderObject(Interface::RangedIcon))->setItemDef(plr->getEquipmentItemId(UnitDefines::Ranged));
	dynamic_pointer_cast<ItemIcon>(getRenderObject(Interface::ChestIcon))->setItemDef(plr->getEquipmentItemId(UnitDefines::Chest));
	dynamic_pointer_cast<ItemIcon>(getRenderObject(Interface::NecklaceIcon))->setItemDef(plr->getEquipmentItemId(UnitDefines::Necklace));
	dynamic_pointer_cast<ItemIcon>(getRenderObject(Interface::GlovesIcon))->setItemDef(plr->getEquipmentItemId(UnitDefines::Hands));
	dynamic_pointer_cast<ItemIcon>(getRenderObject(Interface::Ring1Icon))->setItemDef(plr->getEquipmentItemId(UnitDefines::Ring1));
	dynamic_pointer_cast<ItemIcon>(getRenderObject(Interface::LegsIcon))->setItemDef(plr->getEquipmentItemId(UnitDefines::Legs));
	dynamic_pointer_cast<ItemIcon>(getRenderObject(Interface::BeltIcon))->setItemDef(plr->getEquipmentItemId(UnitDefines::Belt));
	dynamic_pointer_cast<ItemIcon>(getRenderObject(Interface::Ring2Icon))->setItemDef(plr->getEquipmentItemId(UnitDefines::Ring2));
	dynamic_pointer_cast<ItemIcon>(getRenderObject(Interface::FeetIcon))->setItemDef(plr->getEquipmentItemId(UnitDefines::Feet));
	
	string nameupper = plr->getName();
	transform(nameupper.begin(), nameupper.end(), nameupper.begin(), ::toupper);
	dynamic_pointer_cast<TextBoxRo>(getRenderObject(Interface::NameLabel))->setString(nameupper, sf::Color(183, 148, 110, 255));
	
	string subname;
	string rankName = sContentMgr->db("player_exp_levels").data(plr->getLevel(), "name");
	
	if (rankName.empty())
		subname = "Level " + to_string(plr->getLevel()) + " " + PlayerFunctions::className(plr->getClass());
	else
		subname = "Level " + to_string(plr->getLevel()) + " " + PlayerFunctions::className(plr->getClass()) + " (" + rankName + ")";

	dynamic_pointer_cast<TextBoxRo>(getRenderObject(Interface::SubnameLabel))->setString(subname, sf::Color(95, 81, 70, 255));
}

UnitDefines::EquipSlot Equipment::convertInterface(const Interface id)
{
	switch (id)
	{
		case Interface::HelmIcon: return UnitDefines::EquipSlot::Helm;
		case Interface::WeaponIcon: return UnitDefines::EquipSlot::Weapon1;
		case Interface::ShieldIcon: return UnitDefines::EquipSlot::Offhand;
		case Interface::RangedIcon: return UnitDefines::EquipSlot::Ranged;
		case Interface::ChestIcon: return UnitDefines::EquipSlot::Chest;
		case Interface::NecklaceIcon: return UnitDefines::EquipSlot::Necklace;
		case Interface::GlovesIcon: return UnitDefines::EquipSlot::Hands;
		case Interface::Ring1Icon: return UnitDefines::EquipSlot::Ring1;
		case Interface::FeetIcon: return UnitDefines::EquipSlot::Feet;
		case Interface::LegsIcon: return UnitDefines::EquipSlot::Legs;
		case Interface::BeltIcon: return UnitDefines::EquipSlot::Belt;
		case Interface::Ring2Icon: return UnitDefines::EquipSlot::Ring2;
	}

	// Don't try using this function with an unexpected value
	ASSERT(0);
	return UnitDefines::EquipSlot::Helm;
}

void Equipment::givenGabbedIcon(shared_ptr<GameIcon> myIcon, shared_ptr<GameIcon> heldIcon)
{
	// Inventory to Equipment
	if (auto inventory = dynamic_cast<Inventory const*>(heldIcon->getOwner()))
	{
		if (auto ptr = dynamic_pointer_cast<ItemIcon>(heldIcon))
		{
			GP_Client_EquipItem packet;
			packet.m_slotInv = heldIcon->getId();
			packet.m_slotEquip = convertInterface(static_cast<Interface>(myIcon->getId()));
			packet.m_itemId = ptr->getItemDef();
			sConnector->sendPacket(packet.build(StlBuffer{}));
		}
	}
	else
	{
		// Buying from a vendor and some other stuff I guess
		;
	}
}