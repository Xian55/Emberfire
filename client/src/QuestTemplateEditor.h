#pragma once

#include "TableTemplateEditor.h"

class TextBoxRo;
class PromptBox;
class Sprite;
class ClientMap;
class ClientNpc;
class QuestOffer;
class QuestComplete;
class World;
class Game;

class QuestTemplateEditor :	public TableTemplateEditor
{
	enum Interface
	{
		QuestOfferPanel,
		QuestCompletePanel
	};

	enum Field
	{
		QTF_entry, 
		QTF_name, 
		QTF_description,
		QTF_objective, 
		QTF_offer_reward_text, 
		QTF_min_level, 
		QTF_flags, 
		QTF_provided_item, 
		QTF_prev_quest1, 
		QTF_prev_quest2, 
		QTF_prev_quest3, 
		QTF_empty15,
		QTF_start_npc_entry,
		QTF_finish_npc_entry,
		QTF_start_script, 
		QTF_complete_script,
		QTF_empty16,
		QTF_empty1,
		QTF_req_item1, 
		QTF_req_item2, 
		QTF_req_item3, 
		QTF_req_item4, 
		QTF_empty2,
		QTF_req_npc1, 
		QTF_req_npc2, 
		QTF_req_npc3, 
		QTF_req_npc4, 
		QTF_empty3,
		QTF_req_go1, 
		QTF_req_go2, 
		QTF_req_go3, 
		QTF_req_go4, 
		QTF_empty4,
		QTF_req_spell1, 
		QTF_req_spell2, 
		QTF_req_spell3, 
		QTF_req_spell4, 
		QTF_empty5,
		QTF_req_count1, 
		QTF_req_count2, 
		QTF_req_count3, 
		QTF_req_count4, 
		
		QTF_MainFields_End,
		
		QTF_rew_xp, 
		QTF_rew_money, 
		QTF_empty10,
		QTF_rew_choice1_item, 
		QTF_rew_choice1_count, 
		QTF_empty6,
		QTF_rew_choice2_item, 
		QTF_rew_choice2_count, 
		QTF_empty7,
		QTF_rew_choice3_item, 
		QTF_rew_choice3_count, 
		QTF_empty8,
		QTF_rew_choice4_item, 
		QTF_rew_choice4_count, 
		QTF_empty9,
		QTF_rew_item1, 
		QTF_rew_item1_count, 
		QTF_empty11,
		QTF_rew_item2, 
		QTF_rew_item2_count, 
		QTF_empty12,
		QTF_rew_item3,
		QTF_rew_item3_count, 
		QTF_empty13,
		QTF_rew_item4, 
		QTF_rew_item4_count, 
		QTF_empty14,

		NumFields,
	};

	struct FieldDisplay
	{
		shared_ptr<TextBoxRo> label;
		shared_ptr<PromptBox> prompt;
	};

	public:
		QuestTemplateEditor(RenderObjectHolder& owner, const int id);
		virtual ~QuestTemplateEditor();
		
	private:
		void input() final;
		void render() final;
		void notifyCtxMenuClicked(const int id, const string& lineClicked) final;
		
		void initDictionary() override;
		
		bool fieldVisible(const int field) const final;

		int fieldEntry(const int field) const final;

		string fieldTable(const int field) const final;
		string fieldName(const int field) const final;
		string fieldDbName(const int field) const final;
		string fieldKeyName(const int field) const final;
		
		string fieldValueToHumandReadable(const int field, string value) const final;
		string huamnReadableToFieldValue(const int field, const string& readable) const final;

	private:
		void setEntry(const int spellId);

		unique_ptr<ClientMap> m_map;
		
		map<string, string> m_strNpcEntries;
		map<string, string> m_strGoEntries;
		map<string, string> m_strSpellEntries;
		map<string, string> m_strItemEntries;

		shared_ptr<QuestOffer> m_questOffer;
		shared_ptr<QuestComplete> m_questComplete;
		
		unique_ptr<Game> m_fakeGame;
		unique_ptr<World> m_fakeWorld;
};