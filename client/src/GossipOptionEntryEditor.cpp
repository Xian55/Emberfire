#include "stdafx.h"
#include "GossipOptionEntryEditor.h"
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

GossipOptionEntryEditor::GossipOptionEntryEditor(RenderObjectHolder& owner, const int id) :
	TableTemplateEditor(owner, id)
{
	m_numFields = Field::NumFields;
	m_entryFieldId = Field::GOETF_entry;

	setMultiInput(true);
	initDictionary();
}

GossipOptionEntryEditor::~GossipOptionEntryEditor()
{

}

void GossipOptionEntryEditor::input() /*final*/
{
	__super::input();
}

void GossipOptionEntryEditor::render() /*final*/
{
	__super::render();
}

void GossipOptionEntryEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
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

void GossipOptionEntryEditor::initDictionary() /*override*/
{
	__super::initDictionary();
}

void GossipOptionEntryEditor::setEntry(const int entry)
{
	m_entry = entry;

	m_longTxtValues.clear();
	m_longTxtValues.insert(Field::GOETF_text);
	
	m_fieldEnumOptions.clear();
	m_fieldEnumOptions[GOETF_condition1] = &m_strConditions;
	m_fieldEnumOptions[GOETF_condition2] = &m_strConditions;
	m_fieldEnumOptions[GOETF_condition3] = &m_strConditions;
	m_fieldEnumOptions[GOETF_condition1_true] = &m_strBoolean;
	m_fieldEnumOptions[GOETF_condition2_true] = &m_strBoolean;
	m_fieldEnumOptions[GOETF_condition3_true] = &m_strBoolean;

	m_fieldEnumOptions[Field::GOETF_required_npc_flag] = &m_strNpcFlags;

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
	
	populate(Field::GOETF_text, Field::NumFields, getTopLeftCornerRef().x, getTopLeftCornerRef().y, 450, 290);
}
	
bool GossipOptionEntryEditor::fieldVisible(const int field) const
{
	return true;
}

string GossipOptionEntryEditor::fieldDbName(const int field) const
{
	switch (field)
	{
		case GOETF_entry:				   return "entry";
		case GOETF_text:				   return "text";
		case GOETF_required_npc_flag:	   return "required_npc_flag";
		case GOETF_click_new_gossip:	   return "click_new_gossip";
		case GOETF_click_script:		   return "click_script";
		case GOETF_condition1:			   return "condition1";
		case GOETF_condition1_value1:	   return "condition1_value1";
		case GOETF_condition1_value2:	   return "condition1_value2";
		case GOETF_condition1_true:		   return "condition1_true";
		case GOETF_condition2:			   return "condition2";
		case GOETF_condition2_value1:	   return "condition2_value1";
		case GOETF_condition2_value2:	   return "condition2_value2";
		case GOETF_condition2_true:		   return "condition2_true";
		case GOETF_condition3:			   return "condition3";
		case GOETF_condition3_value1:	   return "condition3_value1";
		case GOETF_condition3_value2:	   return "condition3_value2";
		case GOETF_condition3_true:		   return "condition3_true";
	}

	return "unk";
}

string GossipOptionEntryEditor::fieldName(const int field) const
{
	switch (field)
	{
		case GOETF_entry:				   return "Entry";
		case GOETF_text:				   return "Click text";
		case GOETF_required_npc_flag:	   return "Required NpcFlag";
		case GOETF_click_new_gossip:	   return "New gossipId on click";
		case GOETF_click_script:		   return "ScriptId on click";
		case GOETF_condition1:			   return "condition1";
		case GOETF_condition1_value1:	   return "condition1_value1";
		case GOETF_condition1_value2:	   return "condition1_value2";
		case GOETF_condition1_true:		   return "condition1 (true/false)";
		case GOETF_condition2:			   return "condition2";
		case GOETF_condition2_value1:	   return "condition2_value1";
		case GOETF_condition2_value2:	   return "condition2_value2";
		case GOETF_condition2_true:		   return "condition2 (true/false)";
		case GOETF_condition3:			   return "condition2";
		case GOETF_condition3_value1:	   return "condition2_value1";
		case GOETF_condition3_value2:	   return "condition2_value2";
		case GOETF_condition3_true:		   return "condition2 (true/false)";
	}									

	return "unk";
}

int GossipOptionEntryEditor::fieldEntry(const int field) const
{	
	return m_entry;
}

string GossipOptionEntryEditor::fieldKeyName(const int field) const
{
	return fieldDbName(GOETF_entry);
}

string GossipOptionEntryEditor::fieldTable(const int field) const
{
	return "gossip_option";
}

string GossipOptionEntryEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";

	return __super::huamnReadableToFieldValue(field, readable);
}

string GossipOptionEntryEditor::fieldValueToHumandReadable(const int field, string value) const
{
	if (value.empty())
		value = "0";

	return __super::fieldValueToHumandReadable(field, value);
}