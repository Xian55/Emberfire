#include "stdafx.h"
#include "TableTemplateEditor.h"
#include "ContentMgr.h"
#include "TextBoxRo.h"
#include "PromptBox.h"
#include "Application.h"
#include "ConfirmMessageBox.h"
#include "Sprite.h"
#include "SpellIcon.h"
#include "ClientMap.h"
#include "Abilities.h"
#include "SpellVisualKit.h"
#include "Tooltip.h"

#include "..\Files.h"
#include "..\Vector.h"
#include "..\StringHelpers.h"

#include "..\Shared\Config.h"
#include "..\Shared\UnitDefines.h"
#include "..\Shared\Conditions.h"
#include "..\Shared\NpcDefines.h"
#include "..\Shared\SpellDefines.h"
#include "..\Shared\UnitDefines.h"
#include "..\Shared\QuestDefines.h"

#include "..\SqlConnector\SqlConnector.h"
#include "..\SqlConnector\QueryResult.h"

#include <assert.h>

TableTemplateEditor::TableTemplateEditor(RenderObjectHolder& owner, const int id) :
	RenderObjectHolder(&owner, id)
{

}

TableTemplateEditor::~TableTemplateEditor()
{

}
		
void TableTemplateEditor::allowDelete()
{
	m_delButton = make_shared<Button>(*this, "panel_close", Interface::DeleteEntryButton);
	m_delButton->getTopLeftCornerRef().x = 0;
	m_delButton->getTopLeftCornerRef().y = 0;
	addRenderObject(m_delButton);
}

void TableTemplateEditor::input()
{
	ASSERT(m_numFields != -1 && m_entryFieldId != -1);

	if (m_chosenPrompt != 0)
		sApplication->setCurrentPrompt(m_chosenPrompt);

	__super::input();

	processConfirmBox();

	if (isCtxOpen())
		return;

	if (sApplication->keyUp(sf::Keyboard::Escape) && resetCurrentPrompt())
		sApplication->clearKeyUp();

	// Pop prompt box input
	if (sApplication->keyUp(sf::Keyboard::Enter))
	{
		if (auto currentPrompt = reinterpret_cast<PromptBox*>(sApplication->getCurrentPromptInput()))
		{
			if (currentPrompt->getOwner() == this)
			{
				auto field = int(currentPrompt->getId() - Interface::PromptBoxBegin);
				setFieldValue(field, currentPrompt->getContent());
				sApplication->clearKeyUp();
				resetCurrentPrompt();
			}
		}
	}
	
	if (auto confirmBox = sApplication->popConfirmBox({ m_deleteConfirmCode }))
	{
		if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
		{
			if (confirmBox->getCode() == m_deleteConfirmCode)
			{
				deleteEntry();
				setEntry(m_entry);
				return;
			}
		}
	}

	if (m_delButton != nullptr && m_delButton->popActivated())
	{
		int code = ConfirmMessageBox::uniqueCode();
		sApplication->spawnPopup(Util::fmtStr("Delete %d from %s?", getEntry(), fieldTable(m_entryFieldId).c_str()), ConfirmMessageBox::ConfirmBox_YesNo, code);
		m_deleteConfirmCode = code;
	}

	// Mouseover logic for label/prompts
	for (int i = 0; i < m_numFields; ++i)
	{
		auto label = dynamic_pointer_cast<TextBoxRo>(getRenderObject(Interface::LabelsBegin + i));
		auto prompt = dynamic_pointer_cast<PromptBox>(getRenderObject(Interface::PromptBoxBegin + i));

		if (label == nullptr || prompt == nullptr)
			continue;

		// Entire row
		MouseableNode node;
		node.getTopLeftCornerRef().x = min(label->getTopLeftCornerRef().x, prompt->getTopLeftCornerRef().x);
		node.getTopLeftCornerRef().y = min(label->getTopLeftCornerRef().y, prompt->getTopLeftCornerRef().y);
		node.getBottomRightCornerRef().x = max(label->getBottomRightCornerRef().x, prompt->getBottomRightCornerRef().x);
		node.getBottomRightCornerRef().y = max(label->getBottomRightCornerRef().y, prompt->getBottomRightCornerRef().y);

		if (node.isMousedOver(true))
		{
			if (sApplication->mouseUp(sf::Mouse::Left))
			{
				resetCurrentPrompt();
				prompt->setContent(fieldValue(int(i)));
				m_chosenPrompt = prompt.get();
				sContentMgr->playSound("button_change_a.ogg");
			}
			else if (sApplication->mouseUp(sf::Mouse::Right))
			{
				if (m_kitValues.find(int(i)) != m_kitValues.end())
				{
					m_kitEntry = atoi(fieldValue(int(i)).c_str());
					setEntry(m_entry);

					if (m_kitEntry == 0)
					{
						m_newKitField = int(i);
						sApplication->spawnPopup("Assign new kit?", ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_SpellEditorNewKit);	
					}
				}
				else if (m_longTxtValues.find(int(i)) != m_longTxtValues.end())
				{
					string curVal = fieldValue(i);
					Util::simulateInputBox(fieldName(i), curVal);
					setFieldValue(i, curVal);
				}
				else
				{
					resetCurrentPrompt();

					auto options = getFieldCtxOptions(int(i));

					if (!options.empty())
					{
						sContentMgr->playSound("window_target_open_a.ogg");
					
						if (RenderObjectHolder* owner = dynamic_cast<RenderObjectHolder*>(getOwner()))
							owner->registerContextMenuEx(Interface::FieldCtxMenu + int(i), getId(), sApplication->mousePos(), options);
					}
				}
			}
			else
			{
				string description = fieldDescription(i);

				if (!description.empty())
				{
					// Lazy/slow to generate these every frame but we can be lazy for editor tools
					shared_ptr<Tooltip> tt = make_shared<Tooltip>(*sApplication, sf::Vector2i(1450, 25));
					tt->addLine("arial.ttf", 14, description);
					sApplication->setTooltip(tt);
				}
			}
			
			label->getTextRef().setColor(sf::Color::Yellow);
			prompt->setColor(sf::Color::Yellow);
		}
		else if (prompt->isCurrentPrompt())
		{
			label->getTextRef().setColor(sf::Color::Yellow);
			prompt->setColor(sf::Color::Yellow);
		}
		else
		{
			label->getTextRef().setColor(getLabelColor(int(i)));
			prompt->setColor(sf::Color::White);
		}
	}
}

string TableTemplateEditor::fieldDescription(const int field) const
{
	auto itr = m_fieldDescriptions.find(field);

	if (itr != m_fieldDescriptions.end())
		return itr->second;

	return "";
}

vector<pair<string, sf::Color>> TableTemplateEditor::getFieldCtxOptions(const int field) const
{
	vector<string> result;
	vector<pair<string, sf::Color>> exresult;

	auto itr = m_fieldEnumOptions.find(field);

	if (itr != m_fieldEnumOptions.end() && itr->second != nullptr)
	{
		for (auto& stritr : *itr->second)
			result.push_back(stritr.second);
	}
	
	sort(result.begin(), result.end(), Util::compareNaturalSort);

	for (auto& itr : result)
	{
		sf::Color col = sf::Color::White;

		if (isMask(field))
		{
			uint64_t maskVal = _atoi64(fieldValue(field).c_str());
			const uint64_t newFlag = _atoi64(huamnReadableToFieldValue(field, itr).c_str());

			if (maskVal & newFlag)
				col = sf::Color::Green;
		}

		exresult.push_back({itr, col});
	}

	return exresult;
}

void TableTemplateEditor::initDictionary()
{		
	m_strNpcModels.clear();
	auto& sv2 = sContentMgr->db("npc_models");
	
	for (auto& itr : sv2.fetchIndexData())
	{
		string entry = itr.first.c_str();		
		auto nameitr = itr.second.find("name");
				
		if (nameitr != itr.second.end())
		{
			string name = nameitr->second.getString();
			m_strNpcModels[entry] = name;
		}
	}
	
	m_strBoolean[to_string(1)] = "TRUE";
	m_strBoolean[to_string(0)] = "FALSE";

	m_strNpcFlags[to_string(NpcDefines::Flags::Gossip)] = "Gossip";
	m_strNpcFlags[to_string(NpcDefines::Flags::QuestGiver)] = "QuestGiver";
	m_strNpcFlags[to_string(NpcDefines::Flags::Vendor)] = "Vendor";
	m_strNpcFlags[to_string(NpcDefines::Flags::TalkCredit)] = "TalkCredit";
	m_strNpcFlags[to_string(NpcDefines::Flags::LevelToCredit)] = "LevelToCredit";
	m_strNpcFlags[to_string(NpcDefines::Flags::SpendExpCredit)] = "SpendExpCredit";
		
	m_strConditions[to_string(Conditions::Type::Quest_PlayerHasQuest)] = "Quest_PlayerHasQuest";
	m_strConditions[to_string(Conditions::Type::Quest_PlayerHasQuestForItem)] = "Quest_PlayerHasQuestForItem";
	m_strConditions[to_string(Conditions::Type::Quest_PlayerHasOrDidQuest)] = "Quest_PlayerHasOrDidQuest";
	m_strConditions[to_string(Conditions::Type::Quest_PlayerTurnedInQuest)] = "Quest_PlayerTurnedInQuest";
	m_strConditions[to_string(Conditions::Type::Quest_PlayerHasQuestFinishedInLog)] = "Quest_PlayerHasQuestFinishedInLog";
	m_strConditions[to_string(Conditions::Type::Source_Player_HasItemInBag)] = "Source_Player_HasItemInBag";
	m_strConditions[to_string(Conditions::Type::Source_Player_HasItemEquipped)] = "Source_Player_HasItemEquipped";
	m_strConditions[to_string(Conditions::Type::Source_Player_HasItemBagOrEquipped)] = "Source_Player_HasItemBagOrEquipped";
	m_strConditions[to_string(Conditions::Type::Source_Player_LevelGreaterThan)] = "Source_Player_LevelGreaterThan";
	m_strConditions[to_string(Conditions::Type::Source_Unit_HasAura)] = "Source_Unit_HasAura";
	m_strConditions[to_string(Conditions::Type::Target_Player_HasItemInBag)] = "Target_Player_HasItemInBag";
	m_strConditions[to_string(Conditions::Type::Target_Player_HasItemEquipped)] = "Target_Player_HasItemEquipped";
	m_strConditions[to_string(Conditions::Type::Target_Player_HasItemBagOrEquipped)] = "Target_Player_HasItemBagOrEquipped";
	m_strConditions[to_string(Conditions::Type::Target_Unit_HasAura)] = "Target_Unit_HasAura";

	m_strGameFlags[to_string(NpcDefines::GameFlags::NoAggro)] = "NoAggro";
	m_strGameFlags[to_string(NpcDefines::GameFlags::NoTaunt)] = "NoTaunt";
	m_strGameFlags[to_string(NpcDefines::GameFlags::AggroZone)] = "AggroZone";
	m_strGameFlags[to_string(NpcDefines::GameFlags::NoMove)] = "NoMove";
	m_strGameFlags[to_string(NpcDefines::GameFlags::NoFight)] = "NoFight";
	m_strGameFlags[to_string(NpcDefines::GameFlags::NoCallAssist)] = "NoCallAssist";
	m_strGameFlags[to_string(NpcDefines::GameFlags::NoAssist)] = "NoAssist";
	m_strGameFlags[to_string(NpcDefines::GameFlags::NoMelee)] = "NoMelee";

	m_strInterruptCauses[to_string(SpellDefines::InterruptCauses::Movement)] = "Movement";
	m_strInterruptCauses[to_string(SpellDefines::InterruptCauses::StartCasting)] = "StartCasting";
	m_strInterruptCauses[to_string(SpellDefines::InterruptCauses::TakeDamage)] = "TakeDamage";
	m_strInterruptCauses[to_string(SpellDefines::InterruptCauses::TakeDamage_Direct)] = "TakeDamage_Direct";
	m_strInterruptCauses[to_string(SpellDefines::InterruptCauses::DisruptMechanic)] = "DisruptMechanic";
	
	m_strDispelType[to_string(SpellDefines::DispelType::Physical)] = SpellIcon::dispelTypeStr(SpellDefines::DispelType::Physical);
	m_strDispelType[to_string(SpellDefines::DispelType::Magic)] = SpellIcon::dispelTypeStr(SpellDefines::DispelType::Magic);
	m_strDispelType[to_string(SpellDefines::DispelType::Curse)] = SpellIcon::dispelTypeStr(SpellDefines::DispelType::Curse);
	m_strDispelType[to_string(SpellDefines::DispelType::Disease)] = SpellIcon::dispelTypeStr(SpellDefines::DispelType::Disease);
	m_strDispelType[to_string(SpellDefines::DispelType::Poison)] = SpellIcon::dispelTypeStr(SpellDefines::DispelType::Poison);
	
	m_strFactions[to_string(UnitDefines::Faction::PlayerDefault)] = "PlayerDefault";
	m_strFactions[to_string(UnitDefines::Faction::Friendly)] = "Friendly";
	m_strFactions[to_string(UnitDefines::Faction::Neutral)] = "Neutral";
	m_strFactions[to_string(UnitDefines::Faction::Hostile)] = "Hostile";
	m_strFactions[to_string(UnitDefines::Faction::PvP)] = "PvP";
	
	m_strTypes[to_string(NpcDefines::Type::Unknown)] = "Unknown";
	m_strTypes[to_string(NpcDefines::Type::Beast)] = "Beast";
	m_strTypes[to_string(NpcDefines::Type::Dragonkin)] = "Dragonkin";
	m_strTypes[to_string(NpcDefines::Type::Undead)] = "Undead";
	m_strTypes[to_string(NpcDefines::Type::Humanoid)] = "Humanoid";

	m_strAiTypes[to_string(NpcDefines::AiType::MeleeAI)] = "MeleeAI";
	m_strAiTypes[to_string(NpcDefines::AiType::CasterAI)] = "CasterAI";
	m_strAiTypes[to_string(NpcDefines::AiType::ArcherAI)] = "ArcherAI";
	
	m_strMovementTypes[to_string(NpcDefines::DefaultMovement::Idle)] = "Idle";
	m_strMovementTypes[to_string(NpcDefines::DefaultMovement::Random)] = "Random";
	m_strMovementTypes[to_string(NpcDefines::DefaultMovement::Patrol)] = "Patrol";

	m_strQuestFlags[to_string(QuestDefines::Flags::QuestFlagRepeatable)] = "Repeatable";

	m_strAttributes[to_string(SpellDefines::Attributes::AutoApproach)] = "AutoApproach";
	m_strAttributes[to_string(SpellDefines::Attributes::CantTargetSelf)] = "CantTargetSelf";
	m_strAttributes[to_string(SpellDefines::Attributes::CanTargetDead)] = "CanTargetDead";
	m_strAttributes[to_string(SpellDefines::Attributes::CantCrit)] = "CantCrit";
	m_strAttributes[to_string(SpellDefines::Attributes::IgnoreArmor)] = "IgnoreArmor";
	m_strAttributes[to_string(SpellDefines::Attributes::TargetsGround)] = "TargetsGround";
	m_strAttributes[to_string(SpellDefines::Attributes::TargetsItem)] = "TargetsItem";
	m_strAttributes[to_string(SpellDefines::Attributes::IgnoreInvulnerability)] = "IgnoreInvulnerability";
	m_strAttributes[to_string(SpellDefines::Attributes::IgnoreLOS)] = "IgnoreLOS";
	m_strAttributes[to_string(SpellDefines::Attributes::IgnoreResistances)] = "IgnoreResistances";
	m_strAttributes[to_string(SpellDefines::Attributes::ImpossibleBlock)] = "ImpossibleBlock";
	m_strAttributes[to_string(SpellDefines::Attributes::ImpossibleDodge)] = "ImpossibleDodge";
	m_strAttributes[to_string(SpellDefines::Attributes::ImpossibleMiss)] = "ImpossibleMiss";
	m_strAttributes[to_string(SpellDefines::Attributes::ImpossibleParry)] = "ImpossibleParry";
	m_strAttributes[to_string(SpellDefines::Attributes::NoHealBonus)] = "NoHealBonus";
	m_strAttributes[to_string(SpellDefines::Attributes::NoSpellBonus)] = "NoSpellBonus";
	m_strAttributes[to_string(SpellDefines::Attributes::NoThreat)] = "NoThreat";
	m_strAttributes[to_string(SpellDefines::Attributes::NoAggro)] = "NoAggro";
	m_strAttributes[to_string(SpellDefines::Attributes::NotInCombat)] = "NotInCombat";
	m_strAttributes[to_string(SpellDefines::Attributes::OnePerCaster)] = "OnePerCaster";
	m_strAttributes[to_string(SpellDefines::Attributes::OnePerTarget)] = "OnePerTarget";
	m_strAttributes[to_string(SpellDefines::Attributes::Passive)] = "Passive";
	m_strAttributes[to_string(SpellDefines::Attributes::SameStackForAllCasters)] = "SameStackForAllCasters";
	m_strAttributes[to_string(SpellDefines::Attributes::TargetNotInCombat)] = "TargetNotInCombat";
	m_strAttributes[to_string(SpellDefines::Attributes::Triggered)] = "Triggered";
	m_strAttributes[to_string(SpellDefines::Attributes::AnimLockStart)] = "AnimLockStart";
	m_strAttributes[to_string(SpellDefines::Attributes::NoGoLock)] = "NoGoLock";
	m_strAttributes[to_string(SpellDefines::Attributes::DontStopCastingSound)] = "DontStopCastingSound";
	m_strAttributes[to_string(SpellDefines::Attributes::IgnoreStun)] = "IgnoreStun";
	m_strAttributes[to_string(SpellDefines::Attributes::IgnoreIncapacitated)] = "IgnoreIncapacitated";
	m_strAttributes[to_string(SpellDefines::Attributes::IgnoreSleep)] = "IgnoreSleep";
	m_strAttributes[to_string(SpellDefines::Attributes::IgnoreConfused)] = "IgnoreConfused";
	m_strAttributes[to_string(SpellDefines::Attributes::IgnoreFear)] = "IgnoreFear";
	m_strAttributes[to_string(SpellDefines::Attributes::IgnorePolymorph)] = "IgnorePolymorph";
	m_strAttributes[to_string(SpellDefines::Attributes::TargetPlayersOnly)] = "TargetPlayersOnly";
	m_strAttributes[to_string(SpellDefines::Attributes::MouseoverTargeting)] = "MouseoverTargeting";
	m_strAttributes[to_string(SpellDefines::Attributes::PersistsThroughDeath)] = "PersistsThroughDeath";
	m_strAttributes[to_string(SpellDefines::Attributes::NotInArena)] = "NotInArena";
	m_strAttributes[to_string(SpellDefines::Attributes::NotInDungeon)] = "NotInDungeon";
	
	m_strHitResults[to_string(SpellDefines::HitResult::Normal)] = "Block";
	m_strHitResults[to_string(SpellDefines::HitResult::Miss)] = "Miss";
	m_strHitResults[to_string(SpellDefines::HitResult::Resist)] = "Resist";
	m_strHitResults[to_string(SpellDefines::HitResult::Evade)] = "Evade";
	m_strHitResults[to_string(SpellDefines::HitResult::Dodge)] = "Dodge";
	m_strHitResults[to_string(SpellDefines::HitResult::Block)] = "Block";
	m_strHitResults[to_string(SpellDefines::HitResult::Parry)] = "Parry";
	m_strHitResults[to_string(SpellDefines::HitResult::Crit)] = "Crit";
	m_strHitResults[to_string(SpellDefines::HitResult::Immune)] = "Immune";
	
	m_strStats[to_string(UnitDefines::Stat::Mana)] = "Mana";
	m_strStats[to_string(UnitDefines::Stat::Health)] = "Health";
	m_strStats[to_string(UnitDefines::Stat::ArmorValue)] = "ArmorValue";
	m_strStats[to_string(UnitDefines::Stat::Strength)] = "Strength";
	m_strStats[to_string(UnitDefines::Stat::Agility)] = "Agility";
	m_strStats[to_string(UnitDefines::Stat::Willpower)] = "Willpower";
	m_strStats[to_string(UnitDefines::Stat::Intelligence)] = "Intelligence";
	m_strStats[to_string(UnitDefines::Stat::Courage)] = "Courage";
	m_strStats[to_string(UnitDefines::Stat::Regeneration)] = "Regeneration";
	m_strStats[to_string(UnitDefines::Stat::Meditate)] = "Meditate";
	m_strStats[to_string(UnitDefines::Stat::WeaponValue)] = "WeaponValue";
	m_strStats[to_string(UnitDefines::Stat::MeleeCooldown)]	= "MeleeCooldown";
	m_strStats[to_string(UnitDefines::Stat::RangedWeaponValue)] = "RangedWeaponValue";
	m_strStats[to_string(UnitDefines::Stat::RangedCooldown)] = "RangedCooldown";
	m_strStats[to_string(UnitDefines::Stat::MeleeCritical)]	= "MeleeCritical";
	m_strStats[to_string(UnitDefines::Stat::RangedCritical)]= "RangedCritical";
	m_strStats[to_string(UnitDefines::Stat::SpellCritical)]	= "SpellCritical";
	m_strStats[to_string(UnitDefines::Stat::DodgeRating)] = "DodgeRating";
	m_strStats[to_string(UnitDefines::Stat::BlockRating)] = "BlockRating";
	m_strStats[to_string(UnitDefines::Stat::ResistFrost)] = "ResistFrost";
	m_strStats[to_string(UnitDefines::Stat::ResistFire)] = "ResistFire";
	m_strStats[to_string(UnitDefines::Stat::ResistShadow)] = "ResistShadow";
	m_strStats[to_string(UnitDefines::Stat::ResistHoly)] = "ResistHoly";
	m_strStats[to_string(UnitDefines::Stat::Bartering)]	= "Bartering";
	m_strStats[to_string(UnitDefines::Stat::Lockpicking)] = "Lockpicking";
	//m_strStats[to_string(UnitDefines::Stat::Perception)] = "Perception";
	m_strStats[to_string(UnitDefines::Stat::StaffSkill)] = "StaffSkill";
	m_strStats[to_string(UnitDefines::Stat::MaceSkill)]	= "MaceSkill";
	m_strStats[to_string(UnitDefines::Stat::AxesSkill)]	= "AxesSkill";
	m_strStats[to_string(UnitDefines::Stat::SwordSkill)] = "SwordSkill";
	m_strStats[to_string(UnitDefines::Stat::RangedSkill)] = "RangedSkill";
	m_strStats[to_string(UnitDefines::Stat::DaggerSkill)] = "DaggerSkill";
	m_strStats[to_string(UnitDefines::Stat::WandSkill)]	= "WandSkill";
	m_strStats[to_string(UnitDefines::Stat::ShieldSkill)] = "ShieldSkill";
	m_strStats[to_string(UnitDefines::Stat::NpcMeleeSkill)] = "NpcMeleeSkill";
	m_strStats[to_string(UnitDefines::Stat::NpcRangedSkill)] = "NpcRangedSkill";
	m_strStats[to_string(UnitDefines::Stat::ParryChanceBonus)] = "ParryChanceBonus";
	m_strStats[to_string(UnitDefines::Stat::BlockChanceBonus)] = "BlockChanceBonus";
	m_strStats[to_string(UnitDefines::Stat::DodgeChanceBonus)] = "DodgeChanceBonus";

	m_strEffects[to_string(SpellDefines::Effects::SchoolDamage)] = "SchoolDamage";
	m_strEffects[to_string(SpellDefines::Effects::Teleport)] = "Teleport";
	m_strEffects[to_string(SpellDefines::Effects::ApplyAura)] = "ApplyAura";
	m_strEffects[to_string(SpellDefines::Effects::Charge)] = "Charge";
	m_strEffects[to_string(SpellDefines::Effects::ApplyGemSocket)] = "ApplyGemSocket";
	m_strEffects[to_string(SpellDefines::Effects::DestroyGems)] = "DestroyGems";
	m_strEffects[to_string(SpellDefines::Effects::CombineItem)] = "CombineItem"; 
	m_strEffects[to_string(SpellDefines::Effects::ExtractOrb)] = "ExtractOrb"; 
	m_strEffects[to_string(SpellDefines::Effects::ApplyOrbEnchant)] = "ApplyOrbEnchant";
	m_strEffects[to_string(SpellDefines::Effects::Duel)] = "Duel";
	m_strEffects[to_string(SpellDefines::Effects::ManaDrain)] = "ManaDrain";
	m_strEffects[to_string(SpellDefines::Effects::HealthDrain)] = "HealthDrain";
	m_strEffects[to_string(SpellDefines::Effects::Heal)] = "Heal";
	m_strEffects[to_string(SpellDefines::Effects::Resurrect)] = "Resurrect";
	m_strEffects[to_string(SpellDefines::Effects::CreateItem)] = "CreateItem";
	m_strEffects[to_string(SpellDefines::Effects::SummonNpc)] = "SummonNpc";
	m_strEffects[to_string(SpellDefines::Effects::RestoreMana)] = "RestoreMana";
	m_strEffects[to_string(SpellDefines::Effects::Dispel)] = "Dispel";
	m_strEffects[to_string(SpellDefines::Effects::WeaponDamage)] = "WeaponDamage";
	m_strEffects[to_string(SpellDefines::Effects::ManaBurn)] = "ManaBurn";
	m_strEffects[to_string(SpellDefines::Effects::Threat)] = "Threat";
	m_strEffects[to_string(SpellDefines::Effects::TriggerSpell)] = "TriggerSpell";
	m_strEffects[to_string(SpellDefines::Effects::InterruptCast)] = "InterruptCast";
	m_strEffects[to_string(SpellDefines::Effects::SummonObject)] = "SummonObject";
	m_strEffects[to_string(SpellDefines::Effects::ScriptEffect)] = "ScriptEffect";
	m_strEffects[to_string(SpellDefines::Effects::KnockBack)] = "KnockBack";
	m_strEffects[to_string(SpellDefines::Effects::ApplyAreaAura)] = "ApplyAreaAura";
	m_strEffects[to_string(SpellDefines::Effects::HealPct)] = "HealPct";
	m_strEffects[to_string(SpellDefines::Effects::RestoreManaPct)] = "RestoreManaPct";
	m_strEffects[to_string(SpellDefines::Effects::TeleportForward)] = "TeleportForward";
	m_strEffects[to_string(SpellDefines::Effects::MeleeAtk)] = "MeleeAtk";
	m_strEffects[to_string(SpellDefines::Effects::RangedAtk)] = "RangedAtk";
	m_strEffects[to_string(SpellDefines::Effects::LootEffect)] = "LootEffect";
	m_strEffects[to_string(SpellDefines::Effects::Kill)] = "Kill";
	m_strEffects[to_string(SpellDefines::Effects::Gossip)] = "Gossip";
	m_strEffects[to_string(SpellDefines::Effects::Inspect)] = "Inspect";
	m_strEffects[to_string(SpellDefines::Effects::SlideFrom)] = "SlideFrom";
	m_strEffects[to_string(SpellDefines::Effects::LearnSpell)] = "LearnSpell";
	m_strEffects[to_string(SpellDefines::Effects::NearestWp)] = "NearestWp";
	m_strEffects[to_string(SpellDefines::Effects::PullTo)] = "PullTo";
	
	m_strAuraType[to_string(SpellDefines::AuraType::PeriodicDamage)] = "PeriodicDamage";
	m_strAuraType[to_string(SpellDefines::AuraType::PeriodicMeleeDamage)] = "PeriodicMeleeDamage";
	m_strAuraType[to_string(SpellDefines::AuraType::PeriodicHeal)] = "PeriodicHeal";
	m_strAuraType[to_string(SpellDefines::AuraType::PeriodicHealPct)] = "PeriodicHealPct";
	m_strAuraType[to_string(SpellDefines::AuraType::InflictMechanic)] = "InflictMechanic";
	m_strAuraType[to_string(SpellDefines::AuraType::ModifyStat)] = "ModifyStat";
	m_strAuraType[to_string(SpellDefines::AuraType::ModifyStatPct)] = "ModifyStatPct";
	m_strAuraType[to_string(SpellDefines::AuraType::AbsorbDamage)] = "AbsorbDamage";
	m_strAuraType[to_string(SpellDefines::AuraType::ModifyResistance)] = "ModifyResistance";
	m_strAuraType[to_string(SpellDefines::AuraType::PeriodicTriggerSpell)] = "PeriodicTriggerSpell";
	m_strAuraType[to_string(SpellDefines::AuraType::PeriodicRestoreMana)] = "PeriodicRestoreMana";
	m_strAuraType[to_string(SpellDefines::AuraType::PeriodicRestoreManaPct)] = "PeriodicRestoreManaPct";
	m_strAuraType[to_string(SpellDefines::AuraType::PeriodicBurnMana)] = "PeriodicBurnMana";
	m_strAuraType[to_string(SpellDefines::AuraType::ModifyMoveSpeedPct)] = "ModifyMoveSpeedPct";
	m_strAuraType[to_string(SpellDefines::AuraType::MechanicImmunity)] = "MechanicImmunity";
	m_strAuraType[to_string(SpellDefines::AuraType::SchoolImmunity)] = "SchoolImmunity";
	m_strAuraType[to_string(SpellDefines::AuraType::ModifyDamageDealtPct)] = "ModifyDmgDealtPct";
	m_strAuraType[to_string(SpellDefines::AuraType::ModifyDamageReceivedPct)] = "ModifyDmgReceivedPct";
	m_strAuraType[to_string(SpellDefines::AuraType::ModifyHealingDealtPct)] = "ModifyHealingDealtPct";
	m_strAuraType[to_string(SpellDefines::AuraType::ModifyHealingRecvPct)] = "ModifyHealingRecvPct";
	m_strAuraType[to_string(SpellDefines::AuraType::ModifyMeleeSpeedPct)] = "ModifyMeleeSpeedPct";
	m_strAuraType[to_string(SpellDefines::AuraType::ModifyRangedSpeedPct)] = "ModifyRangedSpeedPct";
	m_strAuraType[to_string(SpellDefines::AuraType::Model)] = "Model";
	m_strAuraType[to_string(SpellDefines::AuraType::Proc)] = "Proc";
	m_strAuraType[to_string(SpellDefines::AuraType::RepopOntopOfSelf)] = "RepopOntopOfSelf";
	
	m_strMechanics[to_string(SpellDefines::Mechanics::Confused)] = "Confused";
	m_strMechanics[to_string(SpellDefines::Mechanics::Pacify)] = "Pacify";
	m_strMechanics[to_string(SpellDefines::Mechanics::Fear)] = "Fear";
	m_strMechanics[to_string(SpellDefines::Mechanics::Root)] = "Root";
	m_strMechanics[to_string(SpellDefines::Mechanics::Silence)] = "Silence";
	m_strMechanics[to_string(SpellDefines::Mechanics::Sleep)] = "Sleep";
	m_strMechanics[to_string(SpellDefines::Mechanics::Snare)] = "Snare";
	m_strMechanics[to_string(SpellDefines::Mechanics::Stun)] = "Stun";
	m_strMechanics[to_string(SpellDefines::Mechanics::Incapacitated)] = "Incapacitated";
	m_strMechanics[to_string(SpellDefines::Mechanics::Disrupt)] = "Disrupt";
	m_strMechanics[to_string(SpellDefines::Mechanics::Polymorph)] = "Polymorph";
	m_strMechanics[to_string(SpellDefines::Mechanics::Stealth)] = "Stealth";
			
	m_strUnitAnims[to_string(UnitDefines::AnimId::Stance)] = "Stance";
	m_strUnitAnims[to_string(UnitDefines::AnimId::Spawn)] = "Spawn";
	m_strUnitAnims[to_string(UnitDefines::AnimId::Shoot)] = "Shoot";
	m_strUnitAnims[to_string(UnitDefines::AnimId::Run)] = "Run";
	m_strUnitAnims[to_string(UnitDefines::AnimId::Die)] = "Die";
	m_strUnitAnims[to_string(UnitDefines::AnimId::CritDie)] = "CritDie";
	m_strUnitAnims[to_string(UnitDefines::AnimId::Cast)] = "Cast";
	m_strUnitAnims[to_string(UnitDefines::AnimId::Swing)] = "Swing";
	m_strUnitAnims[to_string(UnitDefines::AnimId::Hit)] = "Hit";
	m_strUnitAnims[to_string(UnitDefines::AnimId::Block)] = "Block";
	m_strUnitAnims[to_string(UnitDefines::AnimId::CastAlt)] = "CastAlt";
	
	m_strBlendModes[to_string(SpellVisualKit::BlendMode::BlendAlpha)] = "BlendAlpha";
	m_strBlendModes[to_string(SpellVisualKit::BlendMode::BlendAdd)] = "BlendAdd";
	m_strBlendModes[to_string(SpellVisualKit::BlendMode::BlendMultiply)] = "BlendMultiply";
	m_strBlendModes[to_string(SpellVisualKit::BlendMode::BlendNone)] = "BlendNone";
	m_strBlendModes[to_string(SpellVisualKit::BlendMode::BlendScreen)] = "BlendScreen";
	
	m_strColors[to_string(sf::Color::White.toInteger())] = "Color_White";
	m_strColors[to_string(sf::Color::Red.toInteger())] = "Color_Red";
	m_strColors[to_string(sf::Color::Green.toInteger())] = "Color_Green";
	m_strColors[to_string(sf::Color::Blue.toInteger())] = "Color_Blue";
	m_strColors[to_string(sf::Color::Yellow.toInteger())] = "Color_Yellow";
	m_strColors[to_string(sf::Color::Magenta.toInteger())] = "Color_Magenta";
	m_strColors[to_string(sf::Color::Cyan.toInteger())] = "Color_Cyan";
	
	{
		sf::Color white(sf::Color::White);  white.a /= 2;
		sf::Color red(sf::Color::Red);  red.a /= 2;
		sf::Color green(sf::Color::Green);  green.a /= 2;
		sf::Color blue(sf::Color::Blue);  blue.a /= 2;
		sf::Color yellow(sf::Color::Yellow);  yellow.a /= 2;
		sf::Color magenta(sf::Color::Magenta); magenta.a /= 2;
		sf::Color cyan(sf::Color::Cyan);  cyan.a /= 2;

		m_strColors[to_string(white.toInteger())] = "Color_White (Half)";
		m_strColors[to_string(red.toInteger())] = "Color_Red (Half)";
		m_strColors[to_string(green.toInteger())] = "Color_Green (Half)";
		m_strColors[to_string(blue.toInteger())] = "Color_Blue (Half)";
		m_strColors[to_string(yellow.toInteger())] = "Color_Yellow (Half)";
		m_strColors[to_string(magenta.toInteger())] = "Color_Magenta (Half)";
		m_strColors[to_string(cyan.toInteger())] = "Color_Cyan (Half)";
	}
	
	{
		sf::Color white(sf::Color::White);  white.a /= 4;
		sf::Color red(sf::Color::Red);  red.a /= 4;
		sf::Color green(sf::Color::Green);  green.a /= 4;
		sf::Color blue(sf::Color::Blue);  blue.a /= 4;
		sf::Color yellow(sf::Color::Yellow);  yellow.a /= 4;
		sf::Color magenta(sf::Color::Magenta); magenta.a /= 4;
		sf::Color cyan(sf::Color::Cyan);  cyan.a /= 4;

		m_strColors[to_string(white.toInteger())] = "Color_White (Quarter)";
		m_strColors[to_string(red.toInteger())] = "Color_Red (Quarter)";
		m_strColors[to_string(green.toInteger())] = "Color_Green (Quarter)";
		m_strColors[to_string(blue.toInteger())] = "Color_Blue (Quarter)";
		m_strColors[to_string(yellow.toInteger())] = "Color_Yellow (Quarter)";
		m_strColors[to_string(magenta.toInteger())] = "Color_Magenta (Quarter)";
		m_strColors[to_string(cyan.toInteger())] = "Color_Cyan (Quarter)";
	}
	
	{
		sf::Color white(sf::Color::White);  white.a /= 8;
		sf::Color red(sf::Color::Red);  red.a /= 8;
		sf::Color green(sf::Color::Green);  green.a /= 8;
		sf::Color blue(sf::Color::Blue);  blue.a /= 8;
		sf::Color yellow(sf::Color::Yellow);  yellow.a /= 8;
		sf::Color magenta(sf::Color::Magenta); magenta.a /= 8;
		sf::Color cyan(sf::Color::Cyan);  cyan.a /= 8;

		m_strColors[to_string(white.toInteger())] = "Color_White (8th)";
		m_strColors[to_string(red.toInteger())] = "Color_Red (8th)";
		m_strColors[to_string(green.toInteger())] = "Color_Green (8th)";
		m_strColors[to_string(blue.toInteger())] = "Color_Blue (8th)";
		m_strColors[to_string(yellow.toInteger())] = "Color_Yellow (8th)";
		m_strColors[to_string(magenta.toInteger())] = "Color_Magenta (8th)";
		m_strColors[to_string(cyan.toInteger())] = "Color_Cyan (8th)";
	}

	m_strTargetType[to_string(SpellDefines::TargetType::Unit_Caster)] = "Unit_Caster";
	m_strTargetType[to_string(SpellDefines::TargetType::Unit_Friendly)] = "Unit_Friendly";
	m_strTargetType[to_string(SpellDefines::TargetType::Unit_Any)] = "Unit_Any";
	m_strTargetType[to_string(SpellDefines::TargetType::Unit_AreaSrc_Friendly)] = "Unit_AreaSrc_Friendly";
	m_strTargetType[to_string(SpellDefines::TargetType::Unit_AreaDst_Friendly)] = "Unit_AreaDst_Friendly";
	m_strTargetType[to_string(SpellDefines::TargetType::Target_GameObject)] = "Target_GameObject";
	m_strTargetType[to_string(SpellDefines::TargetType::Target_Item)] = "Target_Item";
	m_strTargetType[to_string(SpellDefines::TargetType::Unit_Hostile)] = "Unit_Hostile";
	m_strTargetType[to_string(SpellDefines::TargetType::Unit_AreaSrc_Hostile)] = "Unit_AreaSrc_Hostile";
	m_strTargetType[to_string(SpellDefines::TargetType::Unit_AreaDst_Hostile)] = "Unit_AreaDst_Hostile";
	m_strTargetType[to_string(SpellDefines::TargetType::Unit_AreaDst_Friendly_FromDst)] = "Unit_AreaSrc_Friendly_FromDst";
	m_strTargetType[to_string(SpellDefines::TargetType::Unit_AreaDst_Hostile_FromDst)] = "Unit_AreaDst_Hostile_FromDst";
	
	m_strMagicSchool[to_string(UnitDefines::MagicSchool::Physical)] = UnitFunctions::magicSchoolName(UnitDefines::MagicSchool::Physical);
	m_strMagicSchool[to_string(UnitDefines::MagicSchool::Frost)] = UnitFunctions::magicSchoolName(UnitDefines::MagicSchool::Frost);
	m_strMagicSchool[to_string(UnitDefines::MagicSchool::Fire)] = UnitFunctions::magicSchoolName(UnitDefines::MagicSchool::Fire);
	m_strMagicSchool[to_string(UnitDefines::MagicSchool::Shadow)] = UnitFunctions::magicSchoolName(UnitDefines::MagicSchool::Shadow);
	m_strMagicSchool[to_string(UnitDefines::MagicSchool::Holy)] = UnitFunctions::magicSchoolName(UnitDefines::MagicSchool::Holy);

	m_strEquipSlots[to_string(UnitDefines::EquipSlot::Helm)] = UnitFunctions::equipSlotName(UnitDefines::EquipSlot::Helm);
	m_strEquipSlots[to_string(UnitDefines::EquipSlot::Necklace)] = UnitFunctions::equipSlotName(UnitDefines::EquipSlot::Necklace);
	m_strEquipSlots[to_string(UnitDefines::EquipSlot::Chest)] = UnitFunctions::equipSlotName(UnitDefines::EquipSlot::Chest);
	m_strEquipSlots[to_string(UnitDefines::EquipSlot::Belt)] = UnitFunctions::equipSlotName(UnitDefines::EquipSlot::Belt);
	m_strEquipSlots[to_string(UnitDefines::EquipSlot::Legs)] = UnitFunctions::equipSlotName(UnitDefines::EquipSlot::Legs);
	m_strEquipSlots[to_string(UnitDefines::EquipSlot::Feet)] = UnitFunctions::equipSlotName(UnitDefines::EquipSlot::Feet);
	m_strEquipSlots[to_string(UnitDefines::EquipSlot::Hands)] = UnitFunctions::equipSlotName(UnitDefines::EquipSlot::Hands);
	m_strEquipSlots[to_string(UnitDefines::EquipSlot::Ring1)] = UnitFunctions::equipSlotName(UnitDefines::EquipSlot::Ring1);
	m_strEquipSlots[to_string(UnitDefines::EquipSlot::Ring2)] = UnitFunctions::equipSlotName(UnitDefines::EquipSlot::Ring2);
	m_strEquipSlots[to_string(UnitDefines::EquipSlot::Weapon1)] = UnitFunctions::equipSlotName(UnitDefines::EquipSlot::Weapon1);
	m_strEquipSlots[to_string(UnitDefines::EquipSlot::Offhand)] = UnitFunctions::equipSlotName(UnitDefines::EquipSlot::Offhand);
	m_strEquipSlots[to_string(UnitDefines::EquipSlot::Ranged)] = UnitFunctions::equipSlotName(UnitDefines::EquipSlot::Ranged);
	
	m_strAbilitiesTabs[to_string(Abilities::Stage::Miscbook)] = "Misc";
	m_strAbilitiesTabs[to_string(Abilities::Stage::Spellbook)] = "Spells";
}

sf::Color TableTemplateEditor::getLabelColor(const int field) const
{		
	sf::Color colA(58, 135, 214, 255);
	sf::Color colB(93, 189, 254, 255);
	return field % 2 ? colA : colB;
}

shared_ptr<ConfirmMessageBox> TableTemplateEditor::processConfirmBox()
{
	if (auto confirmBox = sApplication->popConfirmBox({ ConfirmMessageBox::ConfirmCode_DbEditorEntry }))
	{
		if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
		{
			shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));
			db->query(m_insertEntryQuery);

			string err = db->error();

			if (!err.empty())
			{
				sApplication->spawnPopup(err, ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);		
			}
			else
			{
				sContentMgr->loadDbTable(db, m_insertTableName);
				setEntry(m_insertEntry);
			}

			return confirmBox;
		}
	}

	return nullptr;
}

void TableTemplateEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
{
	if (lineClicked.empty())
		return;

	if (id >= Interface::FieldCtxMenu)
	{
		int field = int(id - Interface::FieldCtxMenu);

		if (isMask(field))
		{
			uint64_t maskVal = _atoi64(fieldValue(field).c_str());
			const uint64_t newFlag = _atoi64(huamnReadableToFieldValue(field, lineClicked).c_str());

			if (maskVal & newFlag)
				maskVal &= ~newFlag;
			else
				maskVal |= newFlag;
		
			setFieldValue(field, to_string(maskVal));
		}
		else
		{
			setFieldValue(field, huamnReadableToFieldValue(field, lineClicked));
		}
	}
}

string TableTemplateEditor::fieldValueToHumandReadable(const int field, string value) const
{
	if (m_ignoreEnumForReadable.find(field) != m_ignoreEnumForReadable.end())
		return value;

	auto itr = m_fieldEnumOptions.find(field);

	if (itr != m_fieldEnumOptions.end() && itr->second != nullptr)
	{
		if (isMask(field))
		{
			string result;
			uint64_t mask = _atoi64(value.c_str());

			if (mask == 0)
				return value;

			for (auto& subitr : *itr->second) 
			{
				if (mask & (uint64_t(1) << uint64_t(atoi(subitr.first.c_str()) - uint64_t(1))))
					result += subitr.second + ", ";
			}
			
			if (!result.empty())
			{
				result.erase(result.begin() + result.size() - 1);
				result.erase(result.begin() + result.size() - 1);
			}

			return result;
		}
		else
		{
			auto subItr = itr->second->find(value);

			if (subItr != itr->second->end())
				return subItr->second;
		}
	}
	
	Util::strReplaceAll(value, "\n", "");
	Util::strReplaceAll(value, "\t", "");
	return value;
}

string TableTemplateEditor::fieldValue(const int field) const
{
	auto& sv = sContentMgr->db(fieldTable(field));
	return sv.data(fieldEntry(field), fieldDbName(field));
}

bool TableTemplateEditor::fieldVisible(const int field) const
{
	return true;
}
	
bool TableTemplateEditor::isMask(const int field) const
{
	auto itr = m_maskedValues.find(field);

	if (itr == m_maskedValues.end())
		return false;

	return true;
}

string TableTemplateEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";

	auto itr = m_fieldEnumOptions.find(field);

	if (itr != m_fieldEnumOptions.end() && itr->second != nullptr)
	{
		if (isMask(field))
		{
			uint64_t mask = 0;

			for (auto& subitr : *itr->second) 
			{
				if (subitr.second == readable) 
					mask |= (uint64_t(1) << uint64_t(atoi(subitr.first.c_str()) - uint64_t(1)));
			}

			return to_string(mask);
		}
		else
		{			
			for (auto& subitr : *itr->second) 
			{ 
				if (subitr.second == readable)
					return subitr.first; 
			}
		}
	}
	
	return readable;
}

void TableTemplateEditor::setFieldValue(const int field, string str)
{
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	string tableName = fieldTable(field);
	string fieldName = fieldDbName(field);
	string keyName = fieldKeyName(field);

	// If we're changing the entry, but that one already exists, then load it
	if (field == m_entryFieldId)
	{
		if (db->query(Util::fmtStr("SELECT `%s` FROM `%s` WHERE %s = '%s'", keyName.c_str(), tableName.c_str(), keyName.c_str(), str.c_str())))
		{
			if (sContentMgr->db(tableName).data(str, keyName).empty())
				sContentMgr->loadDbTable(db, tableName);

			setEntry(atoi(str.c_str()));
		}
		else
		{
			m_insertTableName = tableName;
			m_insertEntry = atoi(str.c_str());
			m_insertEntryQuery = Util::fmtStr("INSERT INTO `%s` ('%s') VALUES ('%s');", tableName.c_str(), keyName.c_str(), str.c_str());
			sApplication->spawnPopup("Create new entry?", ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_DbEditorEntry);
		}

		return;
	}

	int key = fieldEntry(field);
	
	sContentMgr->editableDb(tableName).indexData(to_string(key), fieldName, str);
	Util::strReplaceAll(str, "'", "''");
	db->query(Util::fmtStr("UPDATE %s SET `%s`='%s' WHERE %s = '%d';", tableName.c_str(), fieldName.c_str(), str.c_str(), keyName.c_str(), key));

	setEntry(m_entry);

	string err = db->error();

	if (!err.empty())
		sApplication->spawnPopup(err, ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);	

	if (field == m_entryFieldId)
		setEntry(atoi(str.c_str()));
}
		
void TableTemplateEditor::copyDictionary(TableTemplateEditor const& cpy)
{
	  m_strInterruptCauses = cpy.m_strInterruptCauses;
	  m_strDispelType = cpy.m_strDispelType;
	  m_strEffects = cpy.m_strEffects;
	  m_strAttributes = cpy.m_strAttributes;
	  m_strAuraType = cpy.m_strAuraType;
	  m_strMechanics = cpy.m_strMechanics;
	  m_strTargetType = cpy.m_strTargetType;
	  m_strMagicSchool = cpy.m_strMagicSchool;
	  m_strEntries = cpy.m_strEntries;
	  m_strStats = cpy.m_strStats;
	  m_strEquipSlots = cpy.m_strEquipSlots;
	  m_strUnitAnims = cpy.m_strUnitAnims;
	  m_strBlendModes = cpy.m_strBlendModes;
	  m_strColors = cpy.m_strColors;
	  m_strHitResults = cpy.m_strHitResults;
	  m_strAbilitiesTabs = cpy.m_strAbilitiesTabs;
	  m_strFactions = cpy.m_strFactions;
	  m_strTypes = cpy.m_strTypes;
	  m_strAiTypes = cpy.m_strAiTypes;
	  m_strMovementTypes = cpy.m_strMovementTypes;
	  m_strNpcFlags = cpy.m_strNpcFlags;
	  m_strGameFlags = cpy.m_strGameFlags;
	  m_strConditions = cpy.m_strConditions;
	  m_strBoolean = cpy.m_strBoolean;
	  m_strNpcModels = cpy.m_strNpcModels;
}

void TableTemplateEditor::deleteEntry()
{
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));
	db->query(Util::fmtStr("DELETE FROM `%s` WHERE `%s` = '%d'", fieldTable(m_entryFieldId).c_str(), fieldDbName(m_entryFieldId).c_str(), m_entry));

	string err = db->error();

	if (!err.empty())
	{
		sApplication->spawnPopup(err, ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);		
	}
	else
	{
		sContentMgr->loadDbTable(db, fieldTable(m_entryFieldId).c_str());

		if (!m_insertTableName.empty())
			sContentMgr->loadDbTable(db, m_insertTableName);

		setEntry(0);
	}
}

bool TableTemplateEditor::resetCurrentPrompt()
{
	if (auto currentPrompt = reinterpret_cast<PromptBox*>(sApplication->getCurrentPromptInput()))
	{
		if (currentPrompt->getOwner() == this)
		{
			auto field = int(currentPrompt->getId() - Interface::PromptBoxBegin);
			auto value = fieldValue(field);
			auto content = fieldValueToHumandReadable(field, value);
			trimHumanReadableEntry(content, field);
			currentPrompt->setContent(content);
			m_chosenPrompt = nullptr;
			return true;
		}
	}

	return false;
}

void TableTemplateEditor::trimHumanReadableEntry(string& content, const int field) const
{
	auto itr = m_entryFormattedTxt.find(field);

	if (itr != m_entryFormattedTxt.end())
	{
		while (content.size() > 2)
		{
			if (content[0] == '-')
			{
				content.erase(content.begin());
				content.erase(content.begin());
				break;
			}

			content.erase(content.begin());
		}
	}
}