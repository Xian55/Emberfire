#include "stdafx.h"
#include "QuestTemplateEditor.h"
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
#include "QuestOffer.h"
#include "QuestComplete.h"
#include "World.h"
#include "Game.h"

#include "..\Files.h"
#include "..\Vector.h"
#include "..\StringHelpers.h"

#include "..\Shared\Config.h"
#include "..\Shared\UnitDefines.h"
#include "..\Shared\Config.h"
#include "..\Shared\NpcDefines.h"

#include "..\SqlConnector\SqlConnector.h"
#include "..\SqlConnector\QueryResult.h"

QuestTemplateEditor::QuestTemplateEditor(RenderObjectHolder& owner, const int id) :
	TableTemplateEditor(owner, id)
{
	m_map = make_unique<ClientMap>(*this, 0);
	m_map->setRenderEmptyCells(true);
	m_map->setAllowCameraDrag(false);
	m_map->setShowClouds(false);
	m_map->loadFromDisk("debug");
	m_map->getCameraRef() = { 1653.f, 419.f };

	m_numFields = Field::NumFields;
	m_entryFieldId = Field::QTF_entry;

	setMultiInput(true);
	initDictionary();
	
	m_fakeGame = make_unique<Game>(*this, 0);
	m_fakeWorld = make_unique<World>(*m_fakeGame, 0);

	setEntry(sConfig->getInt("QuestTemplateEditor", "Entry"));
}

QuestTemplateEditor::~QuestTemplateEditor()
{

}

void QuestTemplateEditor::input() /*final*/
{
	m_map->attemptInput();
	__super::input();
}

void QuestTemplateEditor::render() /*final*/
{	
	m_questOffer->getTopLeftCornerRef() = { 875, 25 };
	m_questComplete->getTopLeftCornerRef() = { 1325, 25 };
	m_map->attemptRender();	
	__super::render();
}

void QuestTemplateEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
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

void QuestTemplateEditor::initDictionary() /*override*/
{
	__super::initDictionary();
}

void QuestTemplateEditor::setEntry(const int spellId)
{
	m_entry = spellId;

	sConfig->setInt("QuestTemplateEditor", "Entry", m_entry);

	m_longTxtValues.clear();
	m_longTxtValues.insert(QTF_description);
	m_longTxtValues.insert(QTF_objective);
	m_longTxtValues.insert(QTF_offer_reward_text);

	m_maskedValues.clear();
	m_maskedValues.insert(QTF_flags);

	m_fieldEnumOptions.clear();
	m_fieldEnumOptions[QTF_entry] = &m_strEntries;
	m_fieldEnumOptions[QTF_prev_quest1] = &m_strEntries;
	m_fieldEnumOptions[QTF_prev_quest2] = &m_strEntries;
	m_fieldEnumOptions[QTF_prev_quest3] = &m_strEntries;

	m_ignoreEnumForReadable.clear();
	m_ignoreEnumForReadable.insert(QTF_entry);
	
	m_fieldEnumOptions[QTF_start_npc_entry] = &m_strNpcEntries;
	m_fieldEnumOptions[QTF_finish_npc_entry] = &m_strNpcEntries;

	m_fieldEnumOptions[QTF_req_npc1] = &m_strNpcEntries;
	m_fieldEnumOptions[QTF_req_npc2] = &m_strNpcEntries;
	m_fieldEnumOptions[QTF_req_npc3] = &m_strNpcEntries;
	m_fieldEnumOptions[QTF_req_npc4] = &m_strNpcEntries;
	
	m_fieldEnumOptions[QTF_req_go1] = &m_strGoEntries;
	m_fieldEnumOptions[QTF_req_go2] = &m_strGoEntries;
	m_fieldEnumOptions[QTF_req_go3] = &m_strGoEntries;
	m_fieldEnumOptions[QTF_req_go4] = &m_strGoEntries;
	
	m_fieldEnumOptions[QTF_req_spell1] = &m_strSpellEntries;
	m_fieldEnumOptions[QTF_req_spell2] = &m_strSpellEntries;
	m_fieldEnumOptions[QTF_req_spell3] = &m_strSpellEntries;
	m_fieldEnumOptions[QTF_req_spell4] = &m_strSpellEntries;
	
	m_fieldEnumOptions[QTF_req_item1] = &m_strItemEntries;
	m_fieldEnumOptions[QTF_req_item2] = &m_strItemEntries;
	m_fieldEnumOptions[QTF_req_item3] = &m_strItemEntries;
	m_fieldEnumOptions[QTF_req_item4] = &m_strItemEntries;
	
	m_fieldEnumOptions[QTF_rew_item1] = &m_strItemEntries;
	m_fieldEnumOptions[QTF_rew_item2] = &m_strItemEntries;
	m_fieldEnumOptions[QTF_rew_item3] = &m_strItemEntries;
	m_fieldEnumOptions[QTF_rew_item4] = &m_strItemEntries;
	
	m_fieldEnumOptions[QTF_rew_choice1_item] = &m_strItemEntries;
	m_fieldEnumOptions[QTF_rew_choice2_item] = &m_strItemEntries;
	m_fieldEnumOptions[QTF_rew_choice3_item] = &m_strItemEntries;
	m_fieldEnumOptions[QTF_rew_choice4_item] = &m_strItemEntries;
	
	m_fieldEnumOptions[QTF_provided_item] = &m_strItemEntries;

	m_fieldEnumOptions[QTF_flags] = &m_strQuestFlags;

	m_entryFormattedTxt.insert(QTF_req_npc1);
	m_entryFormattedTxt.insert(QTF_req_npc2);
	m_entryFormattedTxt.insert(QTF_req_npc3);
	m_entryFormattedTxt.insert(QTF_req_npc4);
	
	m_entryFormattedTxt.insert(QTF_req_go1);
	m_entryFormattedTxt.insert(QTF_req_go2);
	m_entryFormattedTxt.insert(QTF_req_go3);
	m_entryFormattedTxt.insert(QTF_req_go4);
	
	m_entryFormattedTxt.insert(QTF_req_spell1);
	m_entryFormattedTxt.insert(QTF_req_spell2);
	m_entryFormattedTxt.insert(QTF_req_spell3);
	m_entryFormattedTxt.insert(QTF_req_spell4);
	
	m_entryFormattedTxt.insert(QTF_req_item1);
	m_entryFormattedTxt.insert(QTF_req_item2);
	m_entryFormattedTxt.insert(QTF_req_item3);
	m_entryFormattedTxt.insert(QTF_req_item4);
	
	m_entryFormattedTxt.insert(QTF_rew_item1);
	m_entryFormattedTxt.insert(QTF_rew_item2);
	m_entryFormattedTxt.insert(QTF_rew_item3);
	m_entryFormattedTxt.insert(QTF_rew_item4);
	
	m_entryFormattedTxt.insert(QTF_rew_choice1_item);
	m_entryFormattedTxt.insert(QTF_rew_choice2_item);
	m_entryFormattedTxt.insert(QTF_rew_choice3_item);
	m_entryFormattedTxt.insert(QTF_rew_choice4_item);

	m_strEntries.clear();

	auto& sv = sContentMgr->db("quest_template");
	
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
		
	m_strNpcEntries.clear();

	auto& npc_template = sContentMgr->db("npc_template");
	
	for (auto& itr : npc_template.fetchIndexData())
	{
		string entry = itr.first.c_str();		
		auto nameitr = itr.second.find("name");

		if (nameitr != itr.second.end())
		{
			string name = nameitr->second.getString();
			m_strNpcEntries[entry] = Util::fmtStr("%s - %s", entry.c_str(), name.c_str());
		}
	}
		
	m_strGoEntries.clear();

	auto& gameobject_template = sContentMgr->db("gameobject_template");
	
	for (auto& itr : gameobject_template.fetchIndexData())
	{
		string entry = itr.first.c_str();		
		auto nameitr = itr.second.find("name");

		if (nameitr != itr.second.end())
		{
			string name = nameitr->second.getString();
			m_strGoEntries[entry] = Util::fmtStr("%s - %s", entry.c_str(), name.c_str());
		}
	}
		
	m_strSpellEntries.clear();

	auto& spell_template = sContentMgr->db("spell_template");
	
	for (auto& itr : spell_template.fetchIndexData())
	{
		string entry = itr.first.c_str();		
		auto nameitr = itr.second.find("name");

		if (nameitr != itr.second.end())
		{
			string name = nameitr->second.getString();
			m_strSpellEntries[entry] = Util::fmtStr("%s - %s", entry.c_str(), name.c_str());
		}
	}
		
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
			addRenderObject(promptBox);
		}
	};
	
	populate(0, Field::QTF_MainFields_End, 100, 40, 150, 190);
	populate(Field::QTF_MainFields_End + 1, Field::NumFields, 500, 40, 150, 190);

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
		trimHumanReadableEntry(content, i);
		promptBox->setContent(content.empty() ? value : content);
	}
	
	addRenderObject(m_questOffer = make_shared<QuestOffer>(*m_fakeWorld, Interface::QuestOfferPanel));
	addRenderObject(m_questComplete = make_shared<QuestComplete>(*m_fakeWorld, Interface::QuestCompletePanel));

	m_questOffer->setForQuest(m_entry);
	m_questComplete->setForQuest(m_entry);
	
	m_questOffer->setHidden(false);
	m_questComplete->setHidden(false);
}

string QuestTemplateEditor::fieldDbName(const int field) const
{
	switch (field)
	{
		case QTF_entry: 				return "entry"; 
		case QTF_name: 					return "name"; 
		case QTF_description:			return "description";
		case QTF_objective: 			return "objective"; 
		case QTF_offer_reward_text: 	return "offer_reward_text"; 
		case QTF_min_level: 			return "min_level"; 
		case QTF_flags: 				return "flags"; 
		case QTF_prev_quest1: 			return "prev_quest1"; 
		case QTF_prev_quest2: 			return "prev_quest2"; 
		case QTF_prev_quest3: 			return "prev_quest3"; 
		case QTF_req_item1: 			return "req_item1"; 
		case QTF_req_item2: 			return "req_item2"; 
		case QTF_req_item3: 			return "req_item3"; 
		case QTF_req_item4: 			return "req_item4"; 
		case QTF_req_npc1: 				return "req_npc1"; 
		case QTF_req_npc2: 				return "req_npc2"; 
		case QTF_req_npc3: 				return "req_npc3"; 
		case QTF_req_npc4: 				return "req_npc4"; 
		case QTF_req_go1: 				return "req_go1"; 
		case QTF_req_go2: 				return "req_go2"; 
		case QTF_req_go3: 				return "req_go3"; 
		case QTF_req_go4: 				return "req_go4"; 
		case QTF_req_spell1: 			return "req_spell1"; 
		case QTF_req_spell2: 			return "req_spell2"; 
		case QTF_req_spell3: 			return "req_spell3"; 
		case QTF_req_spell4: 			return "req_spell4"; 
		case QTF_req_count1: 			return "req_count1"; 
		case QTF_req_count2: 			return "req_count2"; 
		case QTF_req_count3: 			return "req_count3"; 
		case QTF_req_count4: 			return "req_count4"; 
		case QTF_start_script: 			return "start_script"; 
		case QTF_complete_script:		return "complete_script";
		case QTF_start_npc_entry:		return "start_npc_entry";
		case QTF_finish_npc_entry:		return "finish_npc_entry";

		case QTF_rew_choice1_item: 		return "rew_choice1_item"; 
		case QTF_rew_choice2_item: 		return "rew_choice2_item"; 
		case QTF_rew_choice3_item: 		return "rew_choice3_item"; 
		case QTF_rew_choice4_item: 		return "rew_choice4_item"; 
		case QTF_rew_choice1_count: 	return "rew_choice1_count";
		case QTF_rew_choice2_count: 	return "rew_choice2_count";
		case QTF_rew_choice3_count: 	return "rew_choice3_count";
		case QTF_rew_choice4_count: 	return "rew_choice4_count";
		case QTF_rew_item1: 			return "rew_item1"; 
		case QTF_rew_item2: 			return "rew_item2"; 
		case QTF_rew_item3:				return "rew_item3";
		case QTF_rew_item4: 			return "rew_item4"; 
		case QTF_rew_item1_count: 		return "rew_item1_count"; 
		case QTF_rew_item2_count: 		return "rew_item2_count"; 
		case QTF_rew_item3_count: 		return "rew_item3_count"; 
		case QTF_rew_item4_count: 		return "rew_item4_count"; 
		case QTF_rew_money: 			return "rew_money"; 
		case QTF_rew_xp: 				return "rew_xp";
		case QTF_provided_item:			return "provided_item";
	}

	return "unk";
}

bool QuestTemplateEditor::fieldVisible(const int field) const
{	
	switch (field)
	{
		case QTF_empty1: return false;
		case QTF_empty2: return false;
		case QTF_empty3: return false;
		case QTF_empty4: return false;
		case QTF_empty5: return false;
		case QTF_empty6: return false;
		case QTF_empty7: return false;
		case QTF_empty8: return false;
		case QTF_empty9: return false;
		case QTF_empty10: return false;
		case QTF_empty11: return false;
		case QTF_empty12: return false;
		case QTF_empty13: return false;
		case QTF_empty14: return false;
		case QTF_empty15: return false;
		case QTF_empty16: return false;
	}

	return __super::fieldVisible(field);
}

int QuestTemplateEditor::fieldEntry(const int field) const
{	
	return m_entry;
}

string QuestTemplateEditor::fieldKeyName(const int field) const
{
	return fieldDbName(QTF_entry);
}

string QuestTemplateEditor::fieldTable(const int field) const
{
	return "quest_template";
}

string QuestTemplateEditor::fieldName(const int field) const
{
	switch (field)
	{
		case QTF_entry: 				return "Entry"; 
		case QTF_name: 					return "Name"; 
		case QTF_description:			return "Description";
		case QTF_objective: 			return "Objective"; 
		case QTF_offer_reward_text: 	return "Rewards Text"; 
		case QTF_min_level: 			return "Required Level"; 
		case QTF_flags: 				return "Quest Flags"; 
		case QTF_prev_quest1: 			return "Previous Quest (1)"; 
		case QTF_prev_quest2: 			return "Previous Quest (2)"; 
		case QTF_prev_quest3: 			return "Previous Quest (3)"; 
		case QTF_req_item1: 			return "Required Item (1)"; 
		case QTF_req_item2: 			return "Required Item (2)"; 
		case QTF_req_item3: 			return "Required Item (3)"; 
		case QTF_req_item4: 			return "Required Item (4)"; 
		case QTF_req_npc1: 				return "Required Kill (1)"; 
		case QTF_req_npc2: 				return "Required Kill (2)"; 
		case QTF_req_npc3: 				return "Required Kill (3)"; 
		case QTF_req_npc4: 				return "Required Kill (4)"; 
		case QTF_req_go1: 				return "Required Object (1)"; 
		case QTF_req_go2: 				return "Required Object (2)"; 
		case QTF_req_go3: 				return "Required Object (3)"; 
		case QTF_req_go4: 				return "Required Object (4)"; 
		case QTF_req_spell1: 			return "Required Spell (1)"; 
		case QTF_req_spell2: 			return "Required Spell (2)"; 
		case QTF_req_spell3: 			return "Required Spell (3)"; 
		case QTF_req_spell4: 			return "Required Spell (4)"; 
		case QTF_req_count1: 			return "Amount Required (1)"; 
		case QTF_req_count2: 			return "Amount Required (2)"; 
		case QTF_req_count3: 			return "Amount Required (3)"; 
		case QTF_req_count4: 			return "Amount Required (4)"; 
		case QTF_start_script: 			return "Script (Accept)"; 
		case QTF_complete_script:		return "Script (Turn-In)";
		case QTF_start_npc_entry:		return "Start Npc";
		case QTF_finish_npc_entry:		return "Finish Npc";
		case QTF_provided_item:			return "Provided Item";
		
		case QTF_rew_choice1_item: 		return "Reward Choice (1)"; 
		case QTF_rew_choice2_item: 		return "Reward Choice (2)"; 
		case QTF_rew_choice3_item: 		return "Reward Choice (3)"; 
		case QTF_rew_choice4_item: 		return "Reward Choice (4)"; 
		case QTF_rew_choice1_count: 	return "Stack Size (1)"; 
		case QTF_rew_choice2_count: 	return "Stack Size (2)"; 
		case QTF_rew_choice3_count: 	return "Stack Size (3)"; 
		case QTF_rew_choice4_count: 	return "Stack Size (4)"; 
		case QTF_rew_item1: 			return "Reward Item (1)"; 
		case QTF_rew_item2: 			return "Reward Item (2)"; 
		case QTF_rew_item3:				return "Reward Item (3)";
		case QTF_rew_item4: 			return "Reward Item (4)"; 
		case QTF_rew_item1_count: 		return "Stack Size (1)"; 
		case QTF_rew_item2_count: 		return "Stack Size (2)"; 
		case QTF_rew_item3_count: 		return "Stack Size (3)"; 
		case QTF_rew_item4_count: 		return "Stack Size (4)"; 
		case QTF_rew_money: 			return "Reward (Gold)"; 
		case QTF_rew_xp: 				return "Reward (Exp)"; 
	}									

	return "unk";
}

string QuestTemplateEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";

	//switch (field)
	//{
	//	case NTF_attack_time:
	//	{
	//		string s = readable;
	//		s.erase(std::remove_if(s.begin(), s.end(), isalpha), s.end());
	//		return readable;
	//	}
	//}
	
	return __super::huamnReadableToFieldValue(field, readable);
}

string QuestTemplateEditor::fieldValueToHumandReadable(const int field, string value) const
{
	if (value.empty())
		value = "0";

	//switch (field)
	//{
	//	case NTF_attack_time:
	//	{
	//		return value + " ms";
	//	}
	//}
	
	return __super::fieldValueToHumandReadable(field, value);
}