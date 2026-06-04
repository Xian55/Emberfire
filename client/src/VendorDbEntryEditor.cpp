#include "stdafx.h"
#include "VendorDbEntryEditor.h"
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

VendorDbEntryEditor::VendorDbEntryEditor(RenderObjectHolder& owner, const int id) :
	TableTemplateEditor(owner, id)
{
	m_numFields = Field::NumFields;
	m_entryFieldId = Field::VETF_entry;

	setMultiInput(true);
	//initDictionary();
}

VendorDbEntryEditor::~VendorDbEntryEditor()
{

}

void VendorDbEntryEditor::input() /*final*/
{
	__super::input();
}

void VendorDbEntryEditor::render() /*final*/
{
	__super::render();
}

void VendorDbEntryEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
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

void VendorDbEntryEditor::copyDictionary(TableTemplateEditor const& cpy) /*final*/
{
	__super::copyDictionary(cpy);

	if (auto* ptr = dynamic_cast<VendorDbEntryEditor const*>(&cpy))
		m_strItemEntries = ptr->m_strItemEntries;
}

void VendorDbEntryEditor::initDictionary() /*final*/
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

void VendorDbEntryEditor::setEntry(const int entry)
{
	m_entry = entry;

	m_fieldEnumOptions.clear();
	m_fieldEnumOptions[VETF_item] = &m_strItemEntries;

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
	
	populate(Field::VETF_item, Field::NumFields, getTopLeftCornerRef().x, getTopLeftCornerRef().y, 450, 290);
}
	
bool VendorDbEntryEditor::fieldVisible(const int field) const
{
	return true;
}

string VendorDbEntryEditor::fieldDbName(const int field) const
{
	switch (field)
	{
		case VETF_entry:				return "entry";
		case VETF_item:					return "item";		
		case VETF_max_count:			return "max_count";
		case VETF_restock_cooldown:		return "restock_cooldown";
	}

	return "unk";
}

string VendorDbEntryEditor::fieldName(const int field) const
{
	switch (field)
	{
		case VETF_entry:				return "entry";
		case VETF_item:					return "Item";		
		case VETF_max_count:			return "Limited Quantity";
		case VETF_restock_cooldown:		return "Restock Cooldown";
	}									

	return "unk";
}

int VendorDbEntryEditor::fieldEntry(const int field) const
{	
	return m_entry;
}

string VendorDbEntryEditor::fieldKeyName(const int field) const
{
	return fieldDbName(VETF_entry);
}

string VendorDbEntryEditor::fieldTable(const int field) const
{
	return "npc_vendor";
}

string VendorDbEntryEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";

	return __super::huamnReadableToFieldValue(field, readable);
}

string VendorDbEntryEditor::fieldValueToHumandReadable(const int field, string value) const
{
	if (value.empty())
		value = "0";

	switch (field)
	{
		case VETF_restock_cooldown:
		{
			return value + " ms";
		}
	}
	
	return __super::fieldValueToHumandReadable(field, value);
}