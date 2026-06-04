#pragma once

#include "TableTemplateEditor.h"

class TextBoxRo;
class PromptBox;
class Sprite;
class ClientMap;
class ClientNpc;

class GossipEntryEditor : public TableTemplateEditor
{
	enum Interface
	{
	};

	enum Field
	{
		GETF_entry,
		GETF_text,
		GETF_condition1,
		GETF_condition1_value1,
		GETF_condition1_value2,
		GETF_condition1_true,
		GETF_condition2,
		GETF_condition2_value1,
		GETF_condition2_value2,
		GETF_condition2_true,
		GETF_condition3,
		GETF_condition3_value1,
		GETF_condition3_value2,
		GETF_condition3_true,

		NumFields,
	};

	struct FieldDisplay
	{
		shared_ptr<TextBoxRo> label;
		shared_ptr<PromptBox> prompt;
	};

	public:
		GossipEntryEditor(RenderObjectHolder& owner, const int id);
		virtual ~GossipEntryEditor();
		
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