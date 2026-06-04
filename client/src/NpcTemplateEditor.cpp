#include "stdafx.h"
#include "NpcTemplateEditor.h"
#include "ContentMgr.h"
#include "TextBoxRo.h"
#include "PromptBox.h"
#include "Application.h"
#include "ConfirmMessageBox.h"
#include "Sprite.h"
#include "GameIcon.h"
#include "ClientMap.h"
#include "ClientNpc.h"
#include "SpellVisualKit.h"
#include "Abilities.h"

#include "..\Files.h"
#include "..\Vector.h"
#include "..\StringHelpers.h"

#include "..\Shared\Config.h"
#include "..\Shared\UnitDefines.h"
#include "..\Shared\Config.h"
#include "..\Shared\NpcDefines.h"

#include "..\SqlConnector\SqlConnector.h"
#include "..\SqlConnector\QueryResult.h"

NpcTemplateEditor::NpcTemplateEditor(RenderObjectHolder& owner, const int id) :
	TableTemplateEditor(owner, id)
{
	m_map = make_unique<ClientMap>(*this, 0);
	m_map->setRenderEmptyCells(true);
	m_map->setAllowCameraDrag(true);
	m_map->setShowClouds(false);
	m_map->loadFromDisk("debug");
	m_map->getCameraRef() = { 1553.f, 319.f };

	m_numFields = Field::NumFields;
	m_entryFieldId = Field::NTF_entry;

	allowDelete();
	setMultiInput(true);
	initDictionary();

	setEntry(sConfig->getInt("NpcTemplateEditor", "Entry"));
}

NpcTemplateEditor::~NpcTemplateEditor()
{

}

void NpcTemplateEditor::input() /*final*/
{
	m_map->attemptInput();
	__super::input();
}

void NpcTemplateEditor::render() /*final*/
{
	m_map->attemptRender();	
	__super::render();
}

void NpcTemplateEditor::deleteEntry() /*final*/
{
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));
	db->query(Util::fmtStr("DELETE FROM npc_ai WHERE creature_entry = %d", m_entry));
	db->query(Util::fmtStr("DELETE FROM npc WHERE entry = %d", m_entry));

	__super::deleteEntry();
}

void NpcTemplateEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
{
	if (lineClicked.empty())
		return;

	if (id >= TableTemplateEditor::FieldCtxMenu)
	{
		Field field = Field(id - TableTemplateEditor::FieldCtxMenu);

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

void NpcTemplateEditor::initDictionary() /*override*/
{
	__super::initDictionary();
		
	m_strAiTargetTypes[to_string(SpellDefines::AiTargetType::Target_T_Victim)] = "Target_T_Victim";
	m_strAiTargetTypes[to_string(SpellDefines::AiTargetType::Target_T_RandomHated)] =  "Target_T_RandomHated";
	m_strAiTargetTypes[to_string(SpellDefines::AiTargetType::Target_T_RandomFriendly)] = "Target_T_RandomFriendly";
	m_strAiTargetTypes[to_string(SpellDefines::AiTargetType::Target_T_MissingMostHpFriendly)] = "Target_T_MissingMostHpFriendly";
}

void NpcTemplateEditor::setEntry(const int spellId)
{
	m_entry = spellId;

	if (m_npc != nullptr)
		m_map->removeWorldRenderable(m_npc);

	m_npc = make_shared<ClientNpc>(1, m_map.get());
	m_npc->setEntry(m_entry);
	m_npc->setVariable(ObjDefines::Variable::ModelId, atoi(sContentMgr->db("npc_template").data(m_npc->getEntry(), "model_id").c_str()));
	m_npc->setVariable(ObjDefines::Variable::ModelScale, atoi(sContentMgr->db("npc_template").data(m_npc->getEntry(), "model_scale").c_str()));
	m_npc->setWorldPosition(sf::Vector2f(static_cast<float>(1.5f), static_cast<float>(23.5f)));
	m_map->addWorldRenderable(m_npc);

	sConfig->setInt("NpcTemplateEditor", "Entry", m_entry);

	m_maskedValues.clear();
	m_maskedValues.insert(NTF_mechanic_immune_mask);
	m_maskedValues.insert(NTF_npc_flags);
	m_maskedValues.insert(NTF_game_flags);

	m_fieldEnumOptions.clear();
	m_fieldEnumOptions[NTF_entry] = &m_strEntries;
	m_fieldEnumOptions[NTF_faction] = &m_strFactions;
	m_fieldEnumOptions[NTF_mechanic_immune_mask] = &m_strMechanics;
	m_fieldEnumOptions[NTF_type] = &m_strTypes;
	m_fieldEnumOptions[NTF_ai_type] = &m_strAiTypes;
	m_fieldEnumOptions[NTF_movement_type] = &m_strMovementTypes;
	m_fieldEnumOptions[NTF_model_id] = &m_strNpcModels;
	m_fieldEnumOptions[NTF_npc_flags] = &m_strNpcFlags;
	m_fieldEnumOptions[NTF_game_flags] = &m_strGameFlags;
	m_fieldEnumOptions[NTF_bool_elite] = &m_strBoolean;
	m_fieldEnumOptions[NTF_bool_boss] = &m_strBoolean;
	m_fieldEnumOptions[NTF_spell_1_id] = &m_strSpellEntries;
	m_fieldEnumOptions[NTF_spell_2_id] = &m_strSpellEntries;
	m_fieldEnumOptions[NTF_spell_3_id] = &m_strSpellEntries;
	m_fieldEnumOptions[NTF_spell_4_id] = &m_strSpellEntries;
	m_fieldEnumOptions[NTF_spell_primary] = &m_strSpellEntries;
	m_fieldEnumOptions[NTF_spell_1_targetType] = &m_strAiTargetTypes;
	m_fieldEnumOptions[NTF_spell_2_targetType] = &m_strAiTargetTypes;
	m_fieldEnumOptions[NTF_spell_3_targetType] = &m_strAiTargetTypes;
	m_fieldEnumOptions[NTF_spell_4_targetType] = &m_strAiTargetTypes;

	m_ignoreEnumForReadable.clear();
	m_ignoreEnumForReadable.insert(NTF_entry);

	m_strEntries.clear();
	m_strSpellEntries.clear();

	auto& sv = sContentMgr->db("npc_template");
	
	for (auto& itr : sv.fetchIndexData())
	{
		string entry = itr.first.c_str();		
		auto nameitr = itr.second.find("name");

		if (nameitr != itr.second.end())
		{
			string name = nameitr->second.getString();
			m_strEntries[entry] = Util::fmtStr("%s - %s", entry.c_str(), name.c_str());
		}
	}

	auto& sv2 = sContentMgr->db("spell_template");
	
	for (auto& itr : sv2.fetchIndexData())
	{
		string entry = itr.first.c_str();		
		auto nameitr = itr.second.find("name");
				
		if (nameitr != itr.second.end())
		{
			string name = entry + " - " + nameitr->second.getString();
			m_strSpellEntries[entry] = name;
		}
	}

	auto populate = [&](int start, int end, int xPos, int yPos, int width, int space)
	{
		for (int i = start, count = 0; i < end; ++i, ++count)
		{
			const int charactersize = 18;
			const int yVal = yPos + (count * 20);
			const auto field = Field(i);

			auto text = make_shared<TextBoxRo>(*this, TableTemplateEditor::LabelsBegin + i, FontId::FrizRegular, 400, charactersize);
			text->setString(fieldName(field), getLabelColor(field));
			text->updateTopLeftCorner(sf::Vector2i(xPos, yVal));
			addRenderObject(text);

			auto promptBox = make_shared<PromptBox>(*this, TableTemplateEditor::PromptBoxBegin + i, FontId::Arial, nullptr, sf::Vector2i(xPos + space, yVal), width, sf::Color::White);
			promptBox->setPromptCharacterSize(charactersize);
			promptBox->setMaxPromptString(1024);
			addRenderObject(promptBox);
		}
	};
	
	populate(0, Field::NTF_MainFields_End, 100, 40, 200, 190);
	populate(Field::NTF_health, Field::NumFields, 700, 40, 150, 190);

	for (auto& conditionalLabel : m_conditionalLabels)
	{
		bool met = true;

		// Check if the conditions are met
		for (auto& condition : conditionalLabel.condition)
		{
			if (atoi(fieldValue(condition.first).c_str()) != condition.second)
			{
				met = false;
				break;
			}
		}
		
		if (met)
		{
			for (auto& newData : conditionalLabel.newData)
			{			
				auto label = dynamic_pointer_cast<TextBoxRo>(getRenderObject(TableTemplateEditor::LabelsBegin + newData.field));
			
				// Apply new label
				label->setString(newData.value, getLabelColor(newData.field));
				m_fieldEnumOptions[newData.field] = newData.dictionary;

				if (newData.mask)
					m_maskedValues.insert(newData.field);
			}
		}
	}
	
	for (int i = 0; i < Field::NumFields; ++i)
	{
		auto promptBox = dynamic_pointer_cast<PromptBox>(getRenderObject(TableTemplateEditor::PromptBoxBegin + i));
		auto label = dynamic_pointer_cast<TextBoxRo>(getRenderObject(TableTemplateEditor::LabelsBegin + i));

		if (promptBox == nullptr || label == nullptr)
			continue;

		if (!fieldVisible(Field(i)))
		{
			promptBox->setHidden(true);
			label->setHidden(true);
		}

		auto value = fieldValue(Field(i));
		auto content = fieldValueToHumandReadable(Field(i), value);
		promptBox->setContent(content.empty() ? value : content);
	}
}

bool NpcTemplateEditor::fieldVisible(const int field) const
{
	switch (field)
	{
		case NTF_empty1: return false;
	}

	return __super::fieldVisible(field);
}

string NpcTemplateEditor::fieldDbName(const int field) const
{
	switch (field)
	{
		case NTF_entry:					  return "entry";
		case NTF_name:					  return "name";
		case NTF_portrait:				  return "portrait";
		case NTF_subname:				  return "subname";
		case NTF_model_id:				  return "model_id";
		case NTF_model_scale:			  return "model_scale";
		case NTF_min_level:				  return "min_level";
		case NTF_max_level:				  return "max_level";
		case NTF_faction:				  return "faction";
		case NTF_type:					  return "type";
		case NTF_npc_flags:				  return "npc_flags";
		case NTF_game_flags:			  return "game_flags";
		case NTF_gossip_menu_id:		  return "gossip_menu_id";
		case NTF_movement_type:			  return "movement_type";
		case NTF_path_id:				  return "path_id";
		case NTF_ai_type:				  return "ai_type";
		case NTF_mechanic_immune_mask:	  return "mechanic_immune_mask";
		case NTF_loot_green_chance:		  return "loot_green_chance";
		case NTF_loot_blue_chance:		  return "loot_blue_chance";
		case NTF_loot_gold_chance:		  return "loot_gold_chance";
		case NTF_loot_purple_chance:	  return "loot_purple_chance";
		case NTF_custom_loot:			  return "custom_loot";
		case NTF_custom_gold_ratio:		  return "custom_gold_ratio";
		case NTF_bool_elite:			  return "bool_elite";
		case NTF_bool_boss:				  return "bool_boss";
		case NTF_spell_primary:			  return "spell_primary";
		case NTF_spell_1_id:			  return "spell_1_id";
		case NTF_spell_1_chance:		  return "spell_1_chance";
		case NTF_spell_1_interval:		  return "spell_1_interval";
		case NTF_spell_1_cooldown:		  return "spell_1_cooldown";
		case NTF_spell_1_targetType:	  return "spell_1_targetType";
		case NTF_spell_2_id:			  return "spell_2_id";
		case NTF_spell_2_chance:		  return "spell_2_chance";
		case NTF_spell_2_interval:		  return "spell_2_interval";
		case NTF_spell_2_cooldown:		  return "spell_2_cooldown";
		case NTF_spell_2_targetType:	  return "spell_2_targetType";
		case NTF_spell_3_id:			  return "spell_3_id";
		case NTF_spell_3_chance:		  return "spell_3_chance";
		case NTF_spell_3_interval:		  return "spell_3_interval";
		case NTF_spell_3_cooldown:		  return "spell_3_cooldown";
		case NTF_spell_3_targetType:	  return "spell_3_targetType";
		case NTF_spell_4_id:			  return "spell_4_id";
		case NTF_spell_4_chance:		  return "spell_4_chance";
		case NTF_spell_4_interval:		  return "spell_4_interval";
		case NTF_spell_4_cooldown:		  return "spell_4_cooldown";
		case NTF_spell_4_targetType:	  return "spell_4_targetType";
			
		case NTF_resistance_holy:		  return "resistance_holy";
		case NTF_resistance_frost:		  return "resistance_frost";
		case NTF_resistance_shadow:		  return "resistance_shadow";
		case NTF_resistance_fire:		  return "resistance_fire";
		case NTF_strength:				  return "strength";
		case NTF_agility:				  return "agility";
		case NTF_intellect:				  return "intellect";
		case NTF_willpower:				  return "willpower";
		case NTF_courage:				  return "courage";
		case NTF_armor:					  return "armor";
		case NTF_health:				  return "health";
		case NTF_mana:					  return "mana";
		case NTF_weapon_value:			  return "weapon_value";
		case NTF_ranged_weapon_value:	  return "ranged_weapon_value";
		case NTF_melee_speed:			  return "melee_speed";
		case NTF_ranged_speed:			  return "ranged_speed";
		case NTF_melee_skill:			  return "melee_skill";
		case NTF_ranged_skill:			  return "ranged_skill";
		case NTF_shield_skill:			  return "shield_skill";
	}

	return "unk";
}

int NpcTemplateEditor::fieldEntry(const int field) const
{	
	return m_entry;
}

string NpcTemplateEditor::fieldKeyName(const int field) const
{
	return fieldDbName(NTF_entry);
}

string NpcTemplateEditor::fieldTable(const int field) const
{
	return "npc_template";
}

string NpcTemplateEditor::fieldName(const int field) const
{
	switch (field)
	{
		case NTF_entry:					  return "Entry";
		case NTF_name:					  return "Name";
		case NTF_portrait:				  return "Portrait";
		case NTF_subname:				  return "Subname";
		case NTF_model_id:				  return "Model Id";
		case NTF_model_scale:			  return "Scale";
		case NTF_min_level:				  return "Level (Min)";
		case NTF_max_level:				  return "Level (Max)";
		case NTF_faction:				  return "Faction";
		case NTF_type:					  return "Type";
		case NTF_npc_flags:				  return "NpcFlags";
		case NTF_game_flags:			  return "GameFlags";
		case NTF_gossip_menu_id:		  return "Gossip Id";
		case NTF_movement_type:			  return "MovementType";
		case NTF_path_id:				  return "Path Id";
		case NTF_ai_type:				  return "AI";
		case NTF_mechanic_immune_mask:	  return "Mechanic Immunity";
		case NTF_loot_green_chance:		  return "Loot Chance (Green)";
		case NTF_loot_blue_chance:		  return "Loot Chance (Blue)";
		case NTF_loot_gold_chance:		  return "Loot Chance (Gold)";
		case NTF_loot_purple_chance:	  return "Loot Chance (Purple)";
		case NTF_custom_loot:			  return "Custom Loot";
		case NTF_custom_gold_ratio:		  return "Custom Gold Ratio";
		case NTF_bool_elite:			  return "Elite (Bool)";
		case NTF_bool_boss:				  return "Boss (Bool)";
		case NTF_spell_primary:			  return "Spell (Primary)";
		case NTF_spell_1_id:			  return "Spell_1 Entry";
		case NTF_spell_1_chance:		  return "Spell_1 Chance";
		case NTF_spell_1_interval:		  return "Spell_1 RollCoold";
		case NTF_spell_1_cooldown:		  return "Spell_1 Cooldown";
		case NTF_spell_1_targetType:	  return "Spell_1 TargetType";
		case NTF_spell_2_id:			  return "Spell_2 Entry";
		case NTF_spell_2_chance:		  return "Spell_2 Chance";
		case NTF_spell_2_interval:		  return "Spell_2 RollCoold";
		case NTF_spell_2_cooldown:		  return "Spell_2 Cooldown";
		case NTF_spell_2_targetType:	  return "Spell_2 TargetType";
		case NTF_spell_3_id:			  return "Spell_3 Entry";
		case NTF_spell_3_chance:		  return "Spell_3 Chance";
		case NTF_spell_3_interval:		  return "Spell_3 RollCoold";
		case NTF_spell_3_cooldown:		  return "Spell_3 Cooldown";
		case NTF_spell_3_targetType:	  return "Spell_3 TargetType";
		case NTF_spell_4_id:			  return "Spell_4 Entry";
		case NTF_spell_4_chance:		  return "Spell_4 Chance";
		case NTF_spell_4_interval:		  return "Spell_4 RollCoold";
		case NTF_spell_4_cooldown:		  return "Spell_4 Cooldown";
		case NTF_spell_4_targetType:	  return "Spell_4 TargetType";
			
		case NTF_resistance_holy:		  return "Resistance Holy";
		case NTF_resistance_frost:		  return "Resistance Frost";
		case NTF_resistance_shadow:		  return "Resistance Shadow";
		case NTF_resistance_fire:		  return "Resistance Fire";
		case NTF_strength:				  return "Strength";
		case NTF_agility:				  return "Agility";
		case NTF_intellect:				  return "Intellect";
		case NTF_willpower:				  return "Willpower";
		case NTF_courage:				  return "Courage";
		case NTF_armor:					  return "Armor Value";
		case NTF_health:				  return "Health";
		case NTF_mana:					  return "Mana";
		case NTF_weapon_value:			  return "Weapon Value";
		case NTF_melee_skill:			  return "Melee Skill";
		case NTF_ranged_weapon_value:	  return "Ranged Wep Value";
		case NTF_ranged_skill:			  return "Ranged Skill";
		case NTF_melee_speed:			  return "Melee Speed";
		case NTF_ranged_speed:			  return "Ranged Speed";
		case NTF_shield_skill:			  return "Shield Skill";
	}									

	return "unk";
}

string NpcTemplateEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";

	switch (field)
	{
		case NTF_melee_speed:
		case NTF_ranged_speed:
		{
			string s = readable;
			s.erase(std::remove_if(s.begin(), s.end(), isalpha), s.end());
			return readable;
		}
	}
	
	return __super::huamnReadableToFieldValue(field, readable);
}

string NpcTemplateEditor::fieldValueToHumandReadable(const int field, string value) const
{
	if (value.empty())
		value = "0";

	switch (field)
	{
		case NTF_melee_speed:
		case NTF_ranged_speed:
		case NTF_spell_1_interval:
		case NTF_spell_1_cooldown:
		case NTF_spell_2_interval:
		case NTF_spell_2_cooldown:
		case NTF_spell_3_interval:
		case NTF_spell_3_cooldown:
		case NTF_spell_4_interval:
		case NTF_spell_4_cooldown:
		{
			return value + " ms";
		}
	}
	
	return __super::fieldValueToHumandReadable(field, value);
}