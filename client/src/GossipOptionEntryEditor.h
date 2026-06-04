#pragma once

#include "TableTemplateEditor.h"

class TextBoxRo;
class PromptBox;
class Sprite;
class ClientMap;
class ClientNpc;

class GossipOptionEntryEditor :	public TableTemplateEditor
{
	enum Interface
	{
		EntriesOffset = 12500
	};

	enum Field
	{
		GOETF_entry,
		GOETF_text,
		GOETF_click_new_gossip,
		GOETF_click_script,
		GOETF_required_npc_flag,
		GOETF_condition1,
		GOETF_condition1_value1,
		GOETF_condition1_value2,
		GOETF_condition1_true,
		GOETF_condition2,
		GOETF_condition2_value1,
		GOETF_condition2_value2,
		GOETF_condition2_true,
		GOETF_condition3,
		GOETF_condition3_value1,
		GOETF_condition3_value2,
		GOETF_condition3_true,

		NumFields,
	};

	struct FieldDisplay
	{
		shared_ptr<TextBoxRo> label;
		shared_ptr<PromptBox> prompt;
	};

	public:
		GossipOptionEntryEditor(RenderObjectHolder& owner, const int id);
		virtual ~GossipOptionEntryEditor();
		
		void deleteEntry() final { __super::deleteEntry(); }
		void setEntry(const int spellId) final;	
		void initDictionary() final;
		
		string fieldTable(const int field) const final;

	private:
		void input() final;
		void render() final;
		void notifyCtxMenuClicked(const int id, const string& lineClicked) final;
		
		bool fieldVisible(const int field) const;

		int fieldEntry(const int field) const final;

		string fieldName(const int field) const final;
		string fieldDbName(const int field) const final;
		string fieldKeyName(const int field) const final;
		
		string fieldValueToHumandReadable(const int field, string value) const final;
		string huamnReadableToFieldValue(const int field, const string& readable) const final;
};