#include "stdafx.h"
#include "VendorRandDbEntryEditor.h"
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
#include "..\Shared\ScriptDefines.h"

#include "..\SqlConnector\SqlConnector.h"
#include "..\SqlConnector\QueryResult.h"

VendorRandDbEntryEditor::VendorRandDbEntryEditor(RenderObjectHolder& owner, const int id) :
	TableTemplateEditor(owner, id)
{
	m_numFields = Field::NumFields;
	m_entryFieldId = Field::VRTF_entry;

	setMultiInput(true);
	//initDictionary();
}

VendorRandDbEntryEditor::~VendorRandDbEntryEditor()
{

}

void VendorRandDbEntryEditor::input() /*final*/
{
	__super::input();
}

void VendorRandDbEntryEditor::render() /*final*/
{
	__super::render();
}

void VendorRandDbEntryEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
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

void VendorRandDbEntryEditor::copyDictionary(TableTemplateEditor const& cpy) /*final*/
{
	__super::copyDictionary(cpy);
	
	if (auto* ptr = dynamic_cast<VendorRandDbEntryEditor const*>(&cpy))
	{
		m_strEquipTypes = ptr->m_strEquipTypes;
		m_strWeaponTypes = ptr->m_strWeaponTypes;
		m_strArmorStyles = ptr->m_strArmorStyles;
	}
}

void VendorRandDbEntryEditor::initDictionary() /*final*/
{
	__super::initDictionary();

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

	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Axe)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Axe);
	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Bow)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Bow);
	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Mace)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Mace);
	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Sword)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Sword);
	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Staff)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Staff);
	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Dagger)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Dagger);
	m_strWeaponTypes[to_string(ItemDefines::WeaponType::Wand)] = ItemFunctions::weaponTypeName(ItemDefines::WeaponType::Wand);

	m_strArmorStyles[to_string(ItemDefines::ArmorStyles::NullArmorStyle)] = "Random";
	m_strArmorStyles[to_string(ItemDefines::ArmorStyles::ArmorCasterCloth)] = "ArmorCasterCloth";
	m_strArmorStyles[to_string(ItemDefines::ArmorStyles::ArmorHeavy)] = "ArmorHeavy";	
}

void VendorRandDbEntryEditor::setEntry(const int entry)
{
	m_entry = entry;

	m_fieldEnumOptions.clear();
	m_fieldEnumOptions[VRTF_equip_type] = &m_strEquipTypes;
	m_fieldEnumOptions[VRTF_weapon_type] = &m_strWeaponTypes;
	m_fieldEnumOptions[VRTF_armor_preference] = &m_strArmorStyles;

	auto populate = [&](int start, int end, int xPos, int yPos, int width, int space)
	{
		for (int i = start, count = 0; i < end; ++i, ++count)
		{
			const int charactersize = 18;
			const int yVal = yPos + (count * 20);
			const auto field = Field(i);

			auto text = make_shared<TextBoxRo>(*this, TableTemplateEditor::LabelsBegin + i, "Friz Quadrata Regular.ttf", 400, charactersize);
			text->setString(fieldName(field), getLabelColor(field));
			text->updateTopLeftCorner(sf::Vector2i(xPos, yVal));
			addRenderObject(text);

			auto promptBox = make_shared<PromptBox>(*this, TableTemplateEditor::PromptBoxBegin + i, "arial.ttf", nullptr, sf::Vector2i(xPos + space, yVal), width, sf::Color::White);

			promptBox->setPromptCharacterSize(charactersize);
			promptBox->setMaxPromptString(1024);
			promptBox->setContent(fieldValueToHumandReadable(field, fieldValue(field)));
			addRenderObject(promptBox);
		}
	};
	
	populate(Field::VRTF_min_level, Field::NumFields, getTopLeftCornerRef().x, getTopLeftCornerRef().y, 450, 290);
}
	
bool VendorRandDbEntryEditor::fieldVisible(const int field) const
{
	return true;
}

string VendorRandDbEntryEditor::fieldDbName(const int field) const
{
	switch (field)
	{
		case VRTF_entry:				return "entry";
		case VRTF_min_level:			return "min_level";
		case VRTF_max_level:			return "max_level";
		case VRTF_equip_type:			return "equip_type";
		case VRTF_weapon_type:			return "weapon_type";
		case VRTF_armor_preference:		return "armor_preference";
		case VRTF_green_chance:			return "green_chance";
		case VRTF_blue_chance:			return "blue_chance";
		case VRTF_gold_chance:			return "gold_chance";
		case VRTF_purple_chance:		return "purple_chance";
	}

	return "unk";
}

string VendorRandDbEntryEditor::fieldName(const int field) const
{
	switch (field)
	{
		case VRTF_entry:				return "entry";
		case VRTF_min_level:			return "Level (min)";
		case VRTF_max_level:			return "Level (max)";
		case VRTF_equip_type:			return "EquipType";
		case VRTF_weapon_type:			return "WeaponType";
		case VRTF_armor_preference:		return "Armor Preference";
		case VRTF_green_chance:			return "Green Chance";
		case VRTF_blue_chance:			return "Blue Chance";
		case VRTF_gold_chance:			return "Gold Chance";
		case VRTF_purple_chance:		return "Purple Chance";
	}									

	return "unk";
}

int VendorRandDbEntryEditor::fieldEntry(const int field) const
{	
	return m_entry;
}

string VendorRandDbEntryEditor::fieldKeyName(const int field) const
{
	return fieldDbName(VRTF_entry);
}

string VendorRandDbEntryEditor::fieldTable(const int field) const
{
	return "npc_vendor_random";
}

string VendorRandDbEntryEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";

	return __super::huamnReadableToFieldValue(field, readable);
}

string VendorRandDbEntryEditor::fieldValueToHumandReadable(const int field, string value) const
{
	if (value.empty())
		value = "0";

	return __super::fieldValueToHumandReadable(field, value);
}