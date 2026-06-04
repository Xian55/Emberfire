#pragma once

#include "TableTemplateEditor.h"

class ClientMap;
class GossipEntryEditor;

class MultiTableEditor : public TableTemplateEditor
{
	enum Interface
	{
		AddNewButton = 1,
		EntriesButtonsOffset = 35000
	};

	enum Field
	{
		GTF_key, 
		NumFields
	};

	public:
		MultiTableEditor(RenderObjectHolder& owner, const int id);
		virtual ~MultiTableEditor();
		
	protected:
		void setEntry(const int spellId) final;

		virtual string getTableName() const = 0; // gossip
		virtual string getKeyName() const = 0; // gossipId
		virtual string getConfigName() const = 0;
		virtual string getOrderKey() const;

		virtual shared_ptr<TableTemplateEditor> spawnEntryObj(const int idx, const int entry, TableTemplateEditor const* dictionary) = 0;		

	private:
		void input() final;
		void render() final;
		void notifyCtxMenuClicked(const int id, const string& lineClicked) final;
		void setFieldValue(const int field, string str) final;

		void initDictionary() override;
		
		bool fieldVisible(const int field) const final { return true; }

		int fieldEntry(const int field) const final;

		string fieldTable(const int field) const final;
		string fieldName(const int field) const final;
		string fieldDbName(const int field) const final;
		string fieldKeyName(const int field) const final;
		string fieldValue(const int field) const final;

		string fieldValueToHumandReadable(const int field, string value) const final;
		string huamnReadableToFieldValue(const int field, const string& readable) const final;

	private:
		unique_ptr<ClientMap> m_map;
		vector<pair<shared_ptr<TableTemplateEditor>, shared_ptr<Button>>> m_entries;

		int m_newConfirmCode{0};
		int m_scrollOffset{0};

		pair<int, shared_ptr<TableTemplateEditor>> m_deleteConfirm;
};