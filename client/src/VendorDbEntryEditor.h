#pragma once

#include "TableTemplateEditor.h"

class TextBoxRo;
class PromptBox;
class Sprite;
class ClientMap;
class ClientNpc;

class VendorDbEntryEditor :	public TableTemplateEditor
{
	enum Interface
	{
	};

	enum Field
	{
		VETF_entry,
		VETF_item,
		VETF_max_count,
		VETF_restock_cooldown,

		NumFields,
	};

	struct FieldDisplay
	{
		shared_ptr<TextBoxRo> label;
		shared_ptr<PromptBox> prompt;
	};

	public:
		VendorDbEntryEditor(RenderObjectHolder& owner, const int id);
		virtual ~VendorDbEntryEditor();
		
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

		map<string, string> m_strItemEntries;
};