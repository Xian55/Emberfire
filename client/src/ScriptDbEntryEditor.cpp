#include "stdafx.h"
#include "ScriptDbEntryEditor.h"
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

ScriptDbEntryEditor::ScriptDbEntryEditor(RenderObjectHolder& owner, const int id) :
	TableTemplateEditor(owner, id)
{
	m_numFields = Field::NumFields;
	m_entryFieldId = Field::SETF_entry;

	setMultiInput(true);
	//initDictionary();
}

ScriptDbEntryEditor::~ScriptDbEntryEditor()
{

}

void ScriptDbEntryEditor::input() /*final*/
{
	__super::input();
}

void ScriptDbEntryEditor::render() /*final*/
{
	__super::render();
}

void ScriptDbEntryEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
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

void ScriptDbEntryEditor::copyDictionary(TableTemplateEditor const& cpy) /*final*/
{
	__super::copyDictionary(cpy);

	if (auto* ptr = dynamic_cast<ScriptDbEntryEditor const*>(&cpy))
		m_strCommands = ptr->m_strCommands;
}

void ScriptDbEntryEditor::initDictionary() /*final*/
{
	__super::initDictionary();

	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_CastSpell)] = "ScriptCmd_CastSpell";
	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_KillCredit)] = "ScriptCmd_KillCredit";
	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_Talk)] = "ScriptCmd_Talk";
	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_MoveTo)] = "ScriptCmd_MoveTo";
	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_SetNpcFlag)] = "ScriptCmd_SetNpcFlag";
	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_TeleportTo)] = "ScriptCmd_TeleportTo";
	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_ToggleGameObject)] = "ScriptCmd_ToggleGameObject";
	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_RemoveAura)] = "ScriptCmd_RemoveAura";
	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_CreateItem)] = "ScriptCmd_CreateItem";
	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_LocateNpc)] = "ScriptCmd_LocateNpc";
	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_OpenBank)] = "ScriptCmd_OpenBank";
	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_PromptRespec)] = "ScriptCmd_PromptRespec";
	m_strCommands[to_string(ScriptDefines::CmdTypes::ScriptCmd_QueueArena)] = "ScriptCmd_QueueArena";
	
}

void ScriptDbEntryEditor::setEntry(const int entry)
{
	m_entry = entry;

	m_fieldEnumOptions.clear();
	m_fieldEnumOptions[SETF_command] = &m_strCommands;

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
	
	populate(Field::SETF_delay, Field::NumFields, getTopLeftCornerRef().x, getTopLeftCornerRef().y, 450, 290);
}
	
bool ScriptDbEntryEditor::fieldVisible(const int field) const
{
	return true;
}

string ScriptDbEntryEditor::fieldDbName(const int field) const
{
	switch (field)
	{
		case SETF_entry:				return "entry";
		case SETF_delay:				return "delay";	
		case SETF_command:				return "command";
		case SETF_data1:				return "data1";
		case SETF_data2:				return "data2";
		case SETF_data3:				return "data3";
		case SETF_data4:				return "data4";
		case SETF_data5:				return "data5";
	}

	return "unk";
}

string ScriptDbEntryEditor::fieldName(const int field) const
{
	switch (field)
	{
		case SETF_entry:				return "Entry";
		case SETF_delay:				return "Delay";	
		case SETF_command:				return "Command";
		case SETF_data1:				return "data1";
		case SETF_data2:				return "data2";
		case SETF_data3:				return "data3";
		case SETF_data4:				return "data4";
		case SETF_data5:				return "data5";
	}									

	return "unk";
}

int ScriptDbEntryEditor::fieldEntry(const int field) const
{	
	return m_entry;
}

string ScriptDbEntryEditor::fieldKeyName(const int field) const
{
	return fieldDbName(SETF_entry);
}

string ScriptDbEntryEditor::fieldTable(const int field) const
{
	return "scripts";
}

string ScriptDbEntryEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";

	return __super::huamnReadableToFieldValue(field, readable);
}

string ScriptDbEntryEditor::fieldValueToHumandReadable(const int field, string value) const
{
	if (value.empty())
		value = "0";
	
	switch (field)
	{
		case SETF_delay:
		{
			return value + " ms";
		}
	}

	return __super::fieldValueToHumandReadable(field, value);
}