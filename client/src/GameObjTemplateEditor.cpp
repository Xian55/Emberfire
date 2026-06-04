#include "stdafx.h"
#include "GameObjTemplateEditor.h"
#include "ContentMgr.h"
#include "TextBoxRo.h"
#include "PromptBox.h"
#include "Application.h"
#include "ConfirmMessageBox.h"
#include "Sprite.h"
#include "GameIcon.h"
#include "ClientMap.h"
#include "ClientGameObj.h"
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

GameObjTemplateEditor::GameObjTemplateEditor(RenderObjectHolder& owner, const int id) :
	TableTemplateEditor(owner, id)
{
	m_map = make_unique<ClientMap>(*this, 0);
	m_map->setRenderEmptyCells(true);
	m_map->setAllowCameraDrag(true);
	m_map->setShowClouds(false);
	m_map->loadFromDisk("debug");
	m_map->getCameraRef() = { 1553.f, 319.f };

	m_numFields = Field::NumFields;
	m_entryFieldId = Field::GTF_entry;

	allowDelete();
	setMultiInput(true);
	initDictionary();

	setEntry(sConfig->getInt("GameObjTemplateEditor", "Entry"));
}

GameObjTemplateEditor::~GameObjTemplateEditor()
{

}

void GameObjTemplateEditor::input() /*final*/
{
	m_map->attemptInput();
	__super::input();
}

void GameObjTemplateEditor::render() /*final*/
{
	m_map->attemptRender();	
	__super::render();
}

void GameObjTemplateEditor::deleteEntry() /*final*/
{
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));
	db->query(Util::fmtStr("DELETE FROM npc_ai WHERE creature_entry = %d", m_entry));
	db->query(Util::fmtStr("DELETE FROM npc WHERE entry = %d", m_entry));

	__super::deleteEntry();
}

void GameObjTemplateEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
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

void GameObjTemplateEditor::initDictionary() /*override*/
{
	__super::initDictionary();

	m_strGoFlags[to_string(GoDefines::Flags::ShowName)] = "ShowName";
	m_strGoFlags[to_string(GoDefines::Flags::Uninteractable)] = "Uninteractable";
	m_strGoFlags[to_string(GoDefines::Flags::RenderOnFloor)] = "RenderOnFloor";
	m_strGoFlags[to_string(GoDefines::Flags::Lockpicking)] = "Lockpicking";
	m_strGoFlags[to_string(GoDefines::Flags::Lv2xForLockpick)] = "Lv2xForLockpick";
	m_strGoFlags[to_string(GoDefines::Flags::NoMouseoverBrighten)] = "NoMouseoverBrighten";
	
	m_strGoTypes[to_string(GoDefines::Type::Container)] = "Container";
	m_strGoTypes[to_string(GoDefines::Type::RespawnNode)] = "RespawnNode";
	m_strGoTypes[to_string(GoDefines::Type::Waypoint)] = "Waypoint";
	m_strGoTypes[to_string(GoDefines::Type::MapChanger)] = "MapChanger";

	m_strGoModels.clear();
	auto& sv2 = sContentMgr->db("gameobject_models");
	
	for (auto& itr : sv2.fetchIndexData())
	{
		string entry = itr.first.c_str();
		string name;

		if (name.empty())
		{
			auto nameItr = itr.second.find("name_closed");

			if (nameItr != itr.second.end() && nameItr->second.getString().size() > 1)
				name = nameItr->second.getString();
		}
		
		if (name.empty())
		{
			auto nameItr = itr.second.find("name_open");

			if (nameItr != itr.second.end() && nameItr->second.getString().size() > 1)
				name = nameItr->second.getString();
		}
		
		if (name.empty())
		{
			auto nameItr = itr.second.find("anim_name_closed");

			if (nameItr != itr.second.end() && nameItr->second.getString().size() > 1)
				name = nameItr->second.getString();
		}
		
		if (name.empty())
		{
			auto nameItr = itr.second.find("anim_name_open");

			if (nameItr != itr.second.end() && nameItr->second.getString().size() > 1)
				name = nameItr->second.getString();
		}
		
		m_strGoModels[entry] = Util::fmtStr("%s - %s", entry.c_str(), name.c_str());
	}

	m_strQuestEntries.clear();
	auto& qt = sContentMgr->db("quest_template");
	
	for (auto& itr : qt.fetchIndexData())
	{
		string entry = itr.first.c_str();		
		auto nameitr = itr.second.find("name");
				
		if (nameitr != itr.second.end())
		{
			string name = nameitr->second.getString();
			m_strQuestEntries[entry] = Util::fmtStr("%s - %s", entry.c_str(), name.c_str());
		}
	}

	m_strItemEntries.clear();
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
			m_strItemEntries[entry] = Util::fmtStr("%s - %s", entry.c_str(), name.c_str());
		}
	}

	m_conditionalLabels.push_back(
	{
		{
			{ GTF_type, GoDefines::Type::Container }
		},

		{
			{ GTF_data1, "Bool Despawn", nullptr, false },
			{ GTF_data2, "Item Id", &m_strItemEntries, false },
			{ GTF_data3, "Loot Level", nullptr, false },
			{ GTF_data4, "Green Chance", nullptr, false },
			{ GTF_data5, "Blue Chance", nullptr, false },
			{ GTF_data6, "Gold Chance", nullptr, false },
			{ GTF_data7, "Purple Chance", nullptr, false },
			{ GTF_data8, "Potion Chance", nullptr, false },
			{ GTF_data9, "Gem Chance", nullptr, false },
			{ GTF_data10, "Gold Ratio", nullptr, false },
			{ GTF_data11, "Num Assured Gear", nullptr, false },
		}
	});

	m_conditionalLabels.push_back(
	{
		{
			{ GTF_type, GoDefines::Type::MapChanger }
		},

		{
			{ GTF_data1, "Map Id", nullptr, false },
			{ GTF_data2, "Instanced", nullptr, false },
			{ GTF_data3, "X-Position", nullptr, false },
			{ GTF_data4, "Y-Position", nullptr, false },
		}
	});
}
		
void GameObjTemplateEditor::getFieldsWithItems(vector<pair<int, string>>& output)
{
	output.clear();
	output.push_back({ GoDefines::Type::Container, "data2" });
}

void GameObjTemplateEditor::setEntry(const int entry)
{
	m_entry = entry;

	if (m_gameObj_closed != nullptr)
		m_map->removeWorldRenderable(m_gameObj_closed);

	if (m_gameObj_open != nullptr)
		m_map->removeWorldRenderable(m_gameObj_open);

	m_gameObj_closed = make_shared<ClientGameObj>(1, m_entry, m_map.get());
	m_gameObj_closed->setWorldPosition(sf::Vector2f(static_cast<float>(1.5f), static_cast<float>(23.5f)));
	m_gameObj_closed->setToggleState(GoDefines::ToggleState::Closed);

	m_gameObj_open = make_shared<ClientGameObj>(1, m_entry, m_map.get());
	m_gameObj_open->setWorldPosition(sf::Vector2f(static_cast<float>(8.5f), static_cast<float>(15.5f)));
	m_gameObj_open->setToggleState(GoDefines::ToggleState::Open);
	
	m_map->addWorldRenderable(m_gameObj_closed);
	m_map->addWorldRenderable(m_gameObj_open);

	sConfig->setInt("GameObjTemplateEditor", "Entry", m_entry);
	
	m_maskedValues.clear();
	m_maskedValues.insert(GTF_flags);

	m_fieldEnumOptions.clear();
	m_fieldEnumOptions[GTF_entry] = &m_strEntries;
	m_fieldEnumOptions[GTF_model] = &m_strGoModels;
	m_fieldEnumOptions[GTF_flags] = &m_strGoFlags;
	m_fieldEnumOptions[GTF_type] = &m_strGoTypes;
	m_fieldEnumOptions[GTF_required_item] = &m_strItemEntries;
	m_fieldEnumOptions[GTF_required_quest] = &m_strQuestEntries;

	m_ignoreEnumForReadable.clear();
	m_ignoreEnumForReadable.insert(GTF_entry);

	m_strEntries.clear();

	auto& sv = sContentMgr->db("gameobject_template");
	
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
	
	populate(0, Field::NumFields, 100, 40, 150, 190);

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

string GameObjTemplateEditor::fieldDbName(const int field) const
{
	switch (field)
	{
		case GTF_entry:				return "entry";
		case GTF_name:				return "name";
		case GTF_type:				return "type";
		case GTF_model:				return "model";
		case GTF_flags:				return "flags";
		case GTF_required_quest:	return "required_quest";
		case GTF_required_item:		return "required_item";
		case GTF_data1:				return "data1";
		case GTF_data2:				return "data2";
		case GTF_data3:				return "data3";
		case GTF_data4:				return "data4";
		case GTF_data5:				return "data5";
		case GTF_data6:				return "data6";
		case GTF_data7:				return "data7";
		case GTF_data8:				return "data8";
		case GTF_data9:				return "data9";
		case GTF_data10:			return "data10";
		case GTF_data11:			return "data11";
	}

	return "unk";
}

int GameObjTemplateEditor::fieldEntry(const int field) const
{	
	return m_entry;
}

string GameObjTemplateEditor::fieldKeyName(const int field) const
{
	return fieldDbName(GTF_entry);
}

string GameObjTemplateEditor::fieldTable(const int field) const
{
	return "gameobject_template";
}

string GameObjTemplateEditor::fieldName(const int field) const
{
	switch (field)
	{
		case GTF_entry:				return "Entry";
		case GTF_name:				return "Name";
		case GTF_type:				return "Type";
		case GTF_model:				return "Model";
		case GTF_flags:				return "Flags";
		case GTF_required_quest:	return "Required Quest";
		case GTF_required_item:		return "Required Item";
		case GTF_data1:				return "Data1";
		case GTF_data2:				return "Data2";
		case GTF_data3:				return "Data3";
		case GTF_data4:				return "Data4";
		case GTF_data5:				return "Data5";
		case GTF_data6:				return "Data6";
		case GTF_data7:				return "Data7";
		case GTF_data8:				return "Data8";
		case GTF_data9:				return "Data9";
		case GTF_data10:			return "Data10";
		case GTF_data11:			return "Data11";
	}									

	return "unk";
}

string GameObjTemplateEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";

	return __super::huamnReadableToFieldValue(field, readable);
}

string GameObjTemplateEditor::fieldValueToHumandReadable(const int field, string value) const
{
	if (value.empty())
		value = "0";
		
	return __super::fieldValueToHumandReadable(field, value);
}