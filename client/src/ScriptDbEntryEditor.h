#pragma once

#include "TableTemplateEditor.h"

class TextBoxRo;
class PromptBox;
class Sprite;
class ClientMap;
class ClientNpc;

class ScriptDbEntryEditor :	public TableTemplateEditor
{
	enum Interface
	{
	};

	enum Field
	{
		SETF_entry,
		SETF_delay,
		SETF_command,
		SETF_data1,
		SETF_data2,
		SETF_data3,
		SETF_data4,
		SETF_data5,

		NumFields,
	};

	struct FieldDisplay
	{
		shared_ptr<TextBoxRo> label;
		shared_ptr<PromptBox> prompt;
	};

	public:
		ScriptDbEntryEditor(RenderObjectHolder& owner, const int id);
		virtual ~ScriptDbEntryEditor();
		
		void deleteEntry() final { __super::deleteEntry(); }
		void setEntry(const int spellId) final;
		void copyDictionary(TableTemplateEditor const& cpy) final;		
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

		map<string, string> m_strCommands;
};