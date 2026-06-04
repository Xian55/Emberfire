#include "stdafx.h"
#include "ItemTemplateEditor.h"
#include "ContentMgr.h"
#include "TextBoxRo.h"
#include "PromptBox.h"
#include "Application.h"
#include "ConfirmMessageBox.h"
#include "Sprite.h"
#include "GameIcon.h"
#include "ClientMap.h"
#include "ItemIcon.h"
#include "SpellVisualKit.h"
#include "Abilities.h"

#include "..\Files.h"
#include "..\Vector.h"
#include "..\StringHelpers.h"

#include "..\Shared\Config.h"
#include "..\Shared\UnitDefines.h"
#include "..\Shared\Config.h"
#include "..\Shared\NpcDefines.h"
#include "..\Shared\ItemDefiner.h"

#include "..\SqlConnector\SqlConnector.h"
#include "..\SqlConnector\QueryResult.h"

ItemTemplateEditor::ItemTemplateEditor(RenderObjectHolder& owner, const int id) :
	TableTemplateEditor(owner, id)
{
	m_map = make_unique<ClientMap>(*this, 0);
	m_map->setRenderEmptyCells(true);
	m_map->setAllowCameraDrag(true);
	m_map->setShowClouds(false);
	m_map->resize(200);
	m_map->getCameraRef() = { 1480.f, 107.f };

	m_numFields = Field::NumFields;
	m_entryFieldId = Field::ITF_entry;

	setMultiInput(true);
	initDictionary();

	setEntry(sConfig->getInt("ItemTemplateEditor", "Entry"));
}

ItemTemplateEditor::~ItemTemplateEditor()
{

}

void ItemTemplateEditor::input() /*final*/
{
	m_map->attemptInput();
	__super::input();
}

void ItemTemplateEditor::render() /*final*/
{
	m_map->attemptRender();	
	__super::render();
}

void ItemTemplateEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
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

void ItemTemplateEditor::initDictionary() /*override*/
{
	__super::initDictionary();

	vector<string> iconNames;
	Util::readLinesFromFile("scripts\\text\\icons_misc.txt", iconNames);
	Util::readLinesFromFile("scripts\\text\\icons_gear.txt", iconNames);

	for (auto& name : iconNames)
		m_strIcons[name] = name;

	iconNames.clear();
	Util::readLinesFromFile("scripts\\text\\gear_models.txt", iconNames);

	for (auto& name : iconNames)
		m_strGearModels[name] = name;

	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Axe)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Axe);
	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Bow)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Bow);
	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Mace)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Mace);
	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Sword)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Sword);
	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Staff)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Staff);
	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Dagger)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Dagger);
	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Wand)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Wand);
	
	m_strItemFlags[to_string(ItemDefines::ItemFlags::ItemFlag_QuestItem)] = "ItemFlag_QuestItem";
	m_strItemFlags[to_string(ItemDefines::ItemFlags::ItemFlag_Skillbook)] = "ItemFlag_Skillbook";
	m_strItemFlags[to_string(ItemDefines::ItemFlags::ItemFlag_GoldValueScales)] = "ItemFlag_GoldValueScales";
	
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_HardMaple)] = to_string(ItemDefines::WeaponMaterial::Wep_HardMaple) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_HardMaple);

	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_Bronze)] =			to_string(ItemDefines::WeaponMaterial::Wep_Bronze) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_Bronze);
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_Steel)] =			to_string(ItemDefines::WeaponMaterial::Wep_Steel) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_Steel);
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_Mythril)] =			to_string(ItemDefines::WeaponMaterial::Wep_Mythril) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_Mythril);
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_Emerald)] =			to_string(ItemDefines::WeaponMaterial::Wep_Emerald) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_Emerald);
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_Crystal)] =			to_string(ItemDefines::WeaponMaterial::Wep_Crystal) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_Crystal);
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_Diamond)] =			to_string(ItemDefines::WeaponMaterial::Wep_Diamond) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_Diamond);
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_Titanium)] =			to_string(ItemDefines::WeaponMaterial::Wep_Titanium) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_Titanium);

	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_Aspen)] =			to_string(ItemDefines::WeaponMaterial::Wep_Aspen) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_Aspen);
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_Elm)] =				to_string(ItemDefines::WeaponMaterial::Wep_Elm) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_Elm);
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_Cherry)] =			to_string(ItemDefines::WeaponMaterial::Wep_Cherry) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_Cherry);
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_BlackWalnut)] =		to_string(ItemDefines::WeaponMaterial::Wep_BlackWalnut) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_BlackWalnut);
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_WhiteOak)] =			to_string(ItemDefines::WeaponMaterial::Wep_WhiteOak) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_WhiteOak);
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_HardMaple)] =		to_string(ItemDefines::WeaponMaterial::Wep_HardMaple) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_HardMaple);
	m_strWeaponMaterial[to_string(ItemDefines::WeaponMaterial::Wep_Hickory)] =			to_string(ItemDefines::WeaponMaterial::Wep_Hickory) + " - " + ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial::Wep_Hickory);
																																									  
	m_strArmorType[to_string(ItemDefines::ArmorType::Leather)] =				to_string(ItemDefines::ArmorType::Leather) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Leather);
	m_strArmorType[to_string(ItemDefines::ArmorType::Brigandine)] =				to_string(ItemDefines::ArmorType::Brigandine) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Brigandine);
	m_strArmorType[to_string(ItemDefines::ArmorType::Gambeson)] =				to_string(ItemDefines::ArmorType::Gambeson) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Gambeson);
	m_strArmorType[to_string(ItemDefines::ArmorType::Bronze)] =					to_string(ItemDefines::ArmorType::Bronze) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Bronze);
	m_strArmorType[to_string(ItemDefines::ArmorType::Steel)] =					to_string(ItemDefines::ArmorType::Steel) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Steel);
	m_strArmorType[to_string(ItemDefines::ArmorType::Mythril)] =				to_string(ItemDefines::ArmorType::Mythril) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Mythril);
	m_strArmorType[to_string(ItemDefines::ArmorType::Emerald)] =				to_string(ItemDefines::ArmorType::Emerald) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Emerald);
	m_strArmorType[to_string(ItemDefines::ArmorType::Crystal)] =				to_string(ItemDefines::ArmorType::Crystal) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Crystal);
	m_strArmorType[to_string(ItemDefines::ArmorType::Diamond)] =				to_string(ItemDefines::ArmorType::Diamond) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Diamond);
	m_strArmorType[to_string(ItemDefines::ArmorType::Titanium)] =				to_string(ItemDefines::ArmorType::Titanium) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Titanium);
	m_strArmorType[to_string(ItemDefines::ArmorType::Cotton)] =					to_string(ItemDefines::ArmorType::Cotton) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Cotton);
	m_strArmorType[to_string(ItemDefines::ArmorType::Linen)] =					to_string(ItemDefines::ArmorType::Linen) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Linen);
	m_strArmorType[to_string(ItemDefines::ArmorType::Wool)] =					to_string(ItemDefines::ArmorType::Wool) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Wool);
	m_strArmorType[to_string(ItemDefines::ArmorType::Silk)] =					to_string(ItemDefines::ArmorType::Silk) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Silk);
	m_strArmorType[to_string(ItemDefines::ArmorType::Spiritsilk)] =				to_string(ItemDefines::ArmorType::Spiritsilk) + " - " + ItemFunctions::armorTypeName(ItemDefines::ArmorType::Spiritsilk);
	
	m_strEquipTypes[to_string(ItemDefines::EquipType::Helm)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Helm);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Necklace)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Necklace);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Chest)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Chest);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Belt)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Belt);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Legs)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Legs);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Feet)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Feet);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Hands)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Hands);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Ring)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Ring);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Weapon)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Weapon);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Shield)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Shield);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Ranged)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Ranged);

	m_strQualities[to_string(ItemDefines::Quality::QualityLv0)] = ItemFunctions::qualityName(ItemDefines::Quality::QualityLv0);
	m_strQualities[to_string(ItemDefines::Quality::QualityLv1)] = ItemFunctions::qualityName(ItemDefines::Quality::QualityLv1);
	m_strQualities[to_string(ItemDefines::Quality::QualityLv2)] = ItemFunctions::qualityName(ItemDefines::Quality::QualityLv2);
	m_strQualities[to_string(ItemDefines::Quality::QualityLv3)] = ItemFunctions::qualityName(ItemDefines::Quality::QualityLv3);
	m_strQualities[to_string(ItemDefines::Quality::QualityLv4)] = ItemFunctions::qualityName(ItemDefines::Quality::QualityLv4);
	m_strQualities[to_string(ItemDefines::Quality::QualityLv5)] = ItemFunctions::qualityName(ItemDefines::Quality::QualityLv5);
	
	m_strClasses[to_string(PlayerDefines::Classes::Bishop)] = PlayerFunctions::className(PlayerDefines::Classes::Bishop);
	m_strClasses[to_string(PlayerDefines::Classes::Mage)] = PlayerFunctions::className(PlayerDefines::Classes::Mage);
	m_strClasses[to_string(PlayerDefines::Classes::Ranger)] = PlayerFunctions::className(PlayerDefines::Classes::Ranger);
	m_strClasses[to_string(PlayerDefines::Classes::Paladin)] = PlayerFunctions::className(PlayerDefines::Classes::Paladin);
}

void ItemTemplateEditor::setEntry(const int spellId)
{
	m_entry = spellId;

	sConfig->setInt("ItemTemplateEditor", "Entry", m_entry);

	m_maskedValues.clear();
	m_maskedValues.insert(ITF_flags);

	m_longTxtValues.clear();
	m_longTxtValues.insert(ITF_description);

	m_fieldEnumOptions.clear();
	m_fieldEnumOptions[ITF_entry] = &m_strEntries;
	m_fieldEnumOptions[ITF_icon] = &m_strIcons;
	m_fieldEnumOptions[ITF_model] = &m_strGearModels;
	m_fieldEnumOptions[ITF_spell_1] = &m_strSpellEntries;
	m_fieldEnumOptions[ITF_spell_2] = &m_strSpellEntries;
	m_fieldEnumOptions[ITF_spell_3] = &m_strSpellEntries;
	m_fieldEnumOptions[ITF_spell_4] = &m_strSpellEntries;
	m_fieldEnumOptions[ITF_spell_5] = &m_strSpellEntries;	
	m_fieldEnumOptions[ITF_weapon_type] = &m_strWeaponTypes;
	m_fieldEnumOptions[ITF_armor_type] = &m_strArmorType;
	m_fieldEnumOptions[ITF_equip_type] = &m_strEquipTypes;
	m_fieldEnumOptions[ITF_weapon_material] = &m_strWeaponMaterial;
	m_fieldEnumOptions[ITF_quality] = &m_strQualities;	
	m_fieldEnumOptions[ITF_stat_type1] = &m_strStats;
	m_fieldEnumOptions[ITF_stat_type2] = &m_strStats;
	m_fieldEnumOptions[ITF_stat_type3] = &m_strStats;
	m_fieldEnumOptions[ITF_stat_type4] = &m_strStats;
	m_fieldEnumOptions[ITF_stat_type5] = &m_strStats;
	m_fieldEnumOptions[ITF_stat_type6] = &m_strStats;
	m_fieldEnumOptions[ITF_stat_type7] = &m_strStats;
	m_fieldEnumOptions[ITF_stat_type8] = &m_strStats;
	m_fieldEnumOptions[ITF_stat_type9] = &m_strStats;
	m_fieldEnumOptions[ITF_stat_type10] = &m_strStats;
	m_fieldEnumOptions[ITF_required_class] = &m_strClasses;
	m_fieldEnumOptions[ITF_quest_offer] = &m_strQuestEntries;
	m_fieldEnumOptions[ITF_flags] = &m_strItemFlags;

	m_ignoreEnumForReadable.clear();
	m_ignoreEnumForReadable.insert(ITF_entry);
	
	m_itemIcon = make_shared<ItemIcon>(*this, Interface::ItemEntryIcon, "gameicon40");
	m_itemIcon->getTopLeftCornerRef() = { 35, 44 };
	m_itemIcon->changeEntry(m_entry);
	addRenderObject(m_itemIcon);

	m_strEntries.clear();
	m_strSpellEntries.clear();

	auto& sv = sContentMgr->db("item_template");
	
	for (auto& itr : sv.fetchIndexData())
	{
		string entry = itr.first.c_str();		
		auto nameitr = itr.second.find("name");
		auto isGenerated = itr.second.find("generated");

		// Generated items are too much data for simple selection
		if (isGenerated == itr.second.end() || isGenerated->second.getString() == "1")
			continue;

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
			string name = nameitr->second.getString();
			m_strSpellEntries[entry] = name;
		}
	}

	m_strQuestEntries.clear();

	auto& quest_template = sContentMgr->db("quest_template");
	
	for (auto& itr : quest_template.fetchIndexData())
	{
		string entry = itr.first.c_str();		
		auto nameitr = itr.second.find("name");

		if (nameitr != itr.second.end())
		{
			string name = nameitr->second.getString();
			m_strQuestEntries[entry] = Util::fmtStr("%s - %s", entry.c_str(), name.c_str());
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
	
	populate(0, Field::ITF_MainFields_End, 100, 40, 150, 190);
	populate(Field::ITF_stat_type1, Field::NumFields, 500, 40, 150, 190);
	
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

string ItemTemplateEditor::fieldDbName(const int field) const
{
	switch (field)
	{
		case ITF_entry:				return "entry";
		case ITF_name: 				return "name"; 
		case ITF_description: 		return "description"; 
		case ITF_icon: 				return "icon"; 
		case ITF_icon_sound: 		return "icon_sound"; 
		case ITF_model: 			return "model"; 
		case ITF_required_level: 	return "required_level";
		case ITF_weapon_type:		return "weapon_type";
		case ITF_armor_type: 		return "armor_type"; 
		case ITF_equip_type: 		return "equip_type"; 
		case ITF_quality: 			return "quality"; 
		case ITF_item_level: 		return "item_level"; 
		case ITF_durability: 		return "durability"; 
		case ITF_sell_price: 		return "sell_price"; 
		case ITF_stack_count:		return "stack_count";
		case ITF_num_sockets:		return "num_sockets";
		case ITF_required_class:	return "required_class";
		case ITF_weapon_material:	return "weapon_material";
		case ITF_quest_offer:		return "quest_offer";
		case ITF_flags:				return "flags";
		case ITF_spell_1: 			return "spell_1"; 
		case ITF_spell_2: 			return "spell_2"; 
		case ITF_spell_3: 			return "spell_3"; 
		case ITF_spell_4: 			return "spell_4"; 
		case ITF_spell_5: 			return "spell_5";
		case ITF_generated: 		return "generated"; 
		case ITF_buy_cost_ratio: 	return "buy_cost_ratio"; 
		case ITF_stat_type1: 		return "stat_type1"; 
		case ITF_stat_value1:		return "stat_value1";
		case ITF_stat_type2: 		return "stat_type2"; 
		case ITF_stat_value2:		return "stat_value2";
		case ITF_stat_type3: 		return "stat_type3"; 
		case ITF_stat_value3:		return "stat_value3";
		case ITF_stat_type4: 		return "stat_type4"; 
		case ITF_stat_value4:		return "stat_value4";
		case ITF_stat_type5: 		return "stat_type5"; 
		case ITF_stat_value5:		return "stat_value5";
		case ITF_stat_type6: 		return "stat_type6"; 
		case ITF_stat_value6:		return "stat_value6";
		case ITF_stat_type7: 		return "stat_type7"; 
		case ITF_stat_value7:		return "stat_value7";
		case ITF_stat_type8: 		return "stat_type8"; 
		case ITF_stat_value8:		return "stat_value8";
		case ITF_stat_type9: 		return "stat_type9"; 
		case ITF_stat_value9:		return "stat_value9";
		case ITF_stat_type10:		return "stat_type10";
		case ITF_stat_value10:		return "stat_value10";
	}

	return "unk";
}
	
void ItemTemplateEditor::setFieldValue(const int field, string str) /*final*/
{
	__super::setFieldValue(field, str);
	sItemDefiner->reloadItemEntry(SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path")), m_entry);
	m_itemIcon->refreshTooltip();
}

int ItemTemplateEditor::fieldEntry(const int field) const
{	
	return m_entry;
}

string ItemTemplateEditor::fieldKeyName(const int field) const
{
	return fieldDbName(ITF_entry);
}

string ItemTemplateEditor::fieldTable(const int field) const
{
	return "item_template";
}

string ItemTemplateEditor::fieldName(const int field) const
{
	switch (field)
	{
		case ITF_entry:				return "Entry";
		case ITF_name: 				return "Name"; 
		case ITF_description: 		return "Description"; 
		case ITF_icon: 				return "Icon"; 
		case ITF_icon_sound: 		return "Icon Sound"; 
		case ITF_model: 			return "Model"; 
		case ITF_required_level: 	return "Required Level";
		case ITF_weapon_type:		return "WeaponType";
		case ITF_weapon_material:	return "WeaponMaterial";
		case ITF_armor_type: 		return "ArmorType"; 
		case ITF_equip_type: 		return "EquipType"; 
		case ITF_quality: 			return "Quality";
		case ITF_item_level: 		return "ItemLevel"; 
		case ITF_durability: 		return "Durability";
		case ITF_sell_price: 		return "Sell Price";
		case ITF_stack_count:		return "Stack Count";
		case ITF_num_sockets:		return "Sockets Count";
		case ITF_required_class:	return "Required Class";
		case ITF_quest_offer:		return "Quest Offer";
		case ITF_flags:				return "Flags";
		case ITF_spell_1: 			return "Spell 1";
		case ITF_spell_2: 			return "Spell 2";
		case ITF_spell_3: 			return "Spell 3";
		case ITF_spell_4: 			return "Spell 4";
		case ITF_spell_5: 			return "Spell 5";
		case ITF_generated: 		return "IsGenerated";
		case ITF_buy_cost_ratio: 	return "Buy-Cost Ratio";
		case ITF_stat_type1: 		return "stat_type1"; 
		case ITF_stat_value1:		return "stat_value1";
		case ITF_stat_type2: 		return "stat_type2"; 
		case ITF_stat_value2:		return "stat_value2";
		case ITF_stat_type3: 		return "stat_type3"; 
		case ITF_stat_value3:		return "stat_value3";
		case ITF_stat_type4: 		return "stat_type4"; 
		case ITF_stat_value4:		return "stat_value4";
		case ITF_stat_type5: 		return "stat_type5"; 
		case ITF_stat_value5:		return "stat_value5";
		case ITF_stat_type6: 		return "stat_type6"; 
		case ITF_stat_value6:		return "stat_value6";
		case ITF_stat_type7: 		return "stat_type7"; 
		case ITF_stat_value7:		return "stat_value7";
		case ITF_stat_type8: 		return "stat_type8"; 
		case ITF_stat_value8:		return "stat_value8";
		case ITF_stat_type9: 		return "stat_type9"; 
		case ITF_stat_value9:		return "stat_value9";
		case ITF_stat_type10:		return "stat_type10";
		case ITF_stat_value10:		return "stat_value10";
	}									

	return "unk";
}

string ItemTemplateEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";

	return __super::huamnReadableToFieldValue(field, readable);
}

string ItemTemplateEditor::fieldValueToHumandReadable(const int field, string value) const
{
	if (value.empty())
		value = "0";
		
	return __super::fieldValueToHumandReadable(field, value);
}