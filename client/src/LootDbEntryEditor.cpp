#include "stdafx.h"
#include "LootDbEntryEditor.h"
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

LootDbEntryEditor::LootDbEntryEditor(RenderObjectHolder& owner, const int id) :
	TableTemplateEditor(owner, id)
{
	m_numFields = Field::NumFields;
	m_entryFieldId = Field::LOTF_entry;

	setMultiInput(true);
	//initDictionary();
}

LootDbEntryEditor::~LootDbEntryEditor()
{

}

void LootDbEntryEditor::input() /*final*/
{
	__super::input();
}

void LootDbEntryEditor::render() /*final*/
{
	__super::render();
}

void LootDbEntryEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
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

void LootDbEntryEditor::copyDictionary(TableTemplateEditor const& cpy) /*final*/
{
	__super::copyDictionary(cpy);

	if (auto* ptr = dynamic_cast<LootDbEntryEditor const*>(&cpy))
		m_strItemEntries = ptr->m_strItemEntries;
}

void LootDbEntryEditor::initDictionary() /*override*/
{
	__super::initDictionary();
		
	m_strItemEntries.clear();

	auto& item_template = sContentMgr->db("item_template");
	
	for (auto& itr : item_template.fetchIndexData())
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
			m_strItemEntries[entry] = Util::fmtStr("%s - %s", entry.c_str(), name.c_str());
		}
	}
}

void LootDbEntryEditor::setEntry(const int entry)
{
	m_entry = entry;

	m_fieldEnumOptions.clear();
	m_fieldEnumOptions[LOTF_condition1] = &m_strConditions;
	m_fieldEnumOptions[LOTF_condition2] = &m_strConditions;
	m_fieldEnumOptions[LOTF_condition1_true] = &m_strBoolean;
	m_fieldEnumOptions[LOTF_condition2_true] = &m_strBoolean;
	m_fieldEnumOptions[LOTF_item] = &m_strItemEntries;

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
			promptBox->setContent(fieldValueToHumandReadable(field, fieldValue(field)));
			addRenderObject(promptBox);
		}
	};
	
	populate(Field::LOTF_item, Field::NumFields, getTopLeftCornerRef().x, getTopLeftCornerRef().y, 450, 290);
}
	
bool LootDbEntryEditor::fieldVisible(const int field) const
{
	return true;
}

string LootDbEntryEditor::fieldDbName(const int field) const
{
	switch (field)
	{
		case LOTF_entry:				return "entry";
		case LOTF_item:					return "item";		
		case LOTF_chance:				return "chance";
		case LOTF_count_min:			return "count_min";
		case LOTF_count_max:			return "count_max";
		case LOTF_condition1:			return "condition1";
		case LOTF_condition1_value1:	return "condition1_value1";
		case LOTF_condition1_value2:	return "condition1_value2";
		case LOTF_condition1_true:		return "condition1_true";
		case LOTF_condition2:			return "condition2";
		case LOTF_condition2_value1:	return "condition2_value1";
		case LOTF_condition2_value2:	return "condition2_value2";
		case LOTF_condition2_true:		return "condition2_true";
	}

	return "unk";
}

string LootDbEntryEditor::fieldName(const int field) const
{
	switch (field)
	{
		case LOTF_entry:				return "Entry";
		case LOTF_item:					return "Item";		
		case LOTF_chance:				return "Chance (0-100)";
		case LOTF_count_min:			return "Count (Min)";
		case LOTF_count_max:			return "Count (Max)";
		case LOTF_condition1:			return "condition1";
		case LOTF_condition1_value1:	return "condition1_value1";
		case LOTF_condition1_value2:	return "condition1_value2";
		case LOTF_condition1_true:		return "condition1 (true/false)";
		case LOTF_condition2:			return "condition2";
		case LOTF_condition2_value1:	return "condition2_value1";
		case LOTF_condition2_value2:	return "condition2_value2";
		case LOTF_condition2_true:		return "condition2 (true/false)";
	}									

	return "unk";
}

int LootDbEntryEditor::fieldEntry(const int field) const
{	
	return m_entry;
}

string LootDbEntryEditor::fieldKeyName(const int field) const
{
	return fieldDbName(LOTF_entry);
}

string LootDbEntryEditor::fieldTable(const int field) const
{
	return "loot";
}

string LootDbEntryEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";

	return __super::huamnReadableToFieldValue(field, readable);
}

string LootDbEntryEditor::fieldValueToHumandReadable(const int field, string value) const
{
	if (value.empty())
		value = "0";
	
	return __super::fieldValueToHumandReadable(field, value);
}