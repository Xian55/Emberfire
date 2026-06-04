#include "stdafx.h"
#include "GossipEntryEditor.h"
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

GossipEntryEditor::GossipEntryEditor(RenderObjectHolder& owner, const int id) :
	TableTemplateEditor(owner, id)
{
	m_numFields = Field::NumFields;
	m_entryFieldId = Field::GETF_entry;

	setMultiInput(true);
	initDictionary();
}

GossipEntryEditor::~GossipEntryEditor()
{

}

void GossipEntryEditor::input() /*final*/
{
	__super::input();
}

void GossipEntryEditor::render() /*final*/
{
	__super::render();
}

void GossipEntryEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
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

void GossipEntryEditor::initDictionary() /*final*/
{
	__super::initDictionary();
}

void GossipEntryEditor::setEntry(const int entry)
{
	m_entry = entry;

	m_longTxtValues.clear();
	m_longTxtValues.insert(Field::GETF_text);
	
	m_fieldEnumOptions.clear();
	m_fieldEnumOptions[GETF_condition1] = &m_strConditions;
	m_fieldEnumOptions[GETF_condition2] = &m_strConditions;
	m_fieldEnumOptions[GETF_condition3] = &m_strConditions;
	m_fieldEnumOptions[GETF_condition1_true] = &m_strBoolean;
	m_fieldEnumOptions[GETF_condition2_true] = &m_strBoolean;
	m_fieldEnumOptions[GETF_condition3_true] = &m_strBoolean;

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

			bool dbg = promptBox->getOwner() == this;

			promptBox->setPromptCharacterSize(charactersize);
			promptBox->setMaxPromptString(1024);
			promptBox->setContent(fieldValueToHumandReadable(field, fieldValue(field)));
			addRenderObject(promptBox);
		}
	};
	
	populate(Field::GETF_text, Field::NumFields, getTopLeftCornerRef().x, getTopLeftCornerRef().y, 450, 290);
}
	
bool GossipEntryEditor::fieldVisible(const int field) const
{
	return true;
}

string GossipEntryEditor::fieldDbName(const int field) const
{
	switch (field)
	{
		case GETF_entry:				return "entry";
		case GETF_text:					return "text";
		case GETF_condition1:			return "condition1";
		case GETF_condition1_value1:	return "condition1_value1";
		case GETF_condition1_value2:	return "condition1_value2";
		case GETF_condition1_true:		return "condition1_true";
		case GETF_condition2:			return "condition2";
		case GETF_condition2_value1:	return "condition2_value1";
		case GETF_condition2_value2:	return "condition2_value2";
		case GETF_condition2_true:		return "condition2_true";
		case GETF_condition3:			return "condition3";
		case GETF_condition3_value1:	return "condition3_value1";
		case GETF_condition3_value2:	return "condition3_value2";
		case GETF_condition3_true:		return "condition3_true";
	}

	return "unk";
}

string GossipEntryEditor::fieldName(const int field) const
{
	switch (field)
	{
		case GETF_entry:				return "Entry";
		case GETF_text:					return "Text";
		case GETF_condition1:			return "condition1";
		case GETF_condition1_value1:	return "condition1_value1";
		case GETF_condition1_value2:	return "condition1_value2";
		case GETF_condition1_true:		return "condition1 (true/false)";
		case GETF_condition2:			return "condition2";
		case GETF_condition2_value1:	return "condition2_value1";
		case GETF_condition2_value2:	return "condition2_value2";
		case GETF_condition2_true:		return "condition2 (true/false)";
		case GETF_condition3:			return "condition3";
		case GETF_condition3_value1:	return "condition3_value1";
		case GETF_condition3_value2:	return "condition3_value2";
		case GETF_condition3_true:		return "condition3 (true/false)";
	}									

	return "unk";
}

int GossipEntryEditor::fieldEntry(const int field) const
{	
	return m_entry;
}

string GossipEntryEditor::fieldKeyName(const int field) const
{
	return fieldDbName(GETF_entry);
}

string GossipEntryEditor::fieldTable(const int field) const
{
	return "gossip";
}

string GossipEntryEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";

	return __super::huamnReadableToFieldValue(field, readable);
}

string GossipEntryEditor::fieldValueToHumandReadable(const int field, string value) const
{
	if (value.empty())
		value = "0";

	return __super::fieldValueToHumandReadable(field, value);
}