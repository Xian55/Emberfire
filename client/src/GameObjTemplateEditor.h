#pragma once

#include "TableTemplateEditor.h"

class TextBoxRo;
class PromptBox;
class Sprite;
class ClientMap;
class ClientGameObj;

class GameObjTemplateEditor :	public TableTemplateEditor
{
	enum Interface
	{

	};

	enum Field
	{
		GTF_entry,
		GTF_name,
		GTF_type,
		GTF_model,
		GTF_flags,
		GTF_required_quest,
		GTF_required_item,
		GTF_data1,
		GTF_data2,
		GTF_data3,
		GTF_data4,
		GTF_data5,
		GTF_data6,
		GTF_data7,
		GTF_data8,
		GTF_data9,
		GTF_data10,
		GTF_data11,

		NumFields,
	};

	struct FieldDisplay
	{
		shared_ptr<TextBoxRo> label;
		shared_ptr<PromptBox> prompt;
	};

	public:
		GameObjTemplateEditor(RenderObjectHolder& owner, const int id);
		virtual ~GameObjTemplateEditor();

		// type, fieldName
		static void getFieldsWithItems(vector<pair<int, string>>& output);
		
	private:
		void input() final;
		void render() final;
		void notifyCtxMenuClicked(const int id, const string& lineClicked) final;
		void deleteEntry() final;

		void initDictionary() override;

		int fieldEntry(const int field) const final;

		string fieldTable(const int field) const final;
		string fieldName(const int field) const final;
		string fieldDbName(const int field) const final;
		string fieldKeyName(const int field) const final;
		
		string fieldValueToHumandReadable(const int field, string value) const final;
		string huamnReadableToFieldValue(const int field, const string& readable) const final;

	private:
		void setEntry(const int spellId);
		
		map<string, string> m_strGoModels;
		map<string, string> m_strGoFlags;
		map<string, string> m_strGoTypes;
		
		map<string, string> m_strItemEntries;
		map<string, string> m_strQuestEntries;

		// Used for demonstrating spell visuals
		unique_ptr<ClientMap> m_map;
		shared_ptr<ClientGameObj> m_gameObj_closed;
		shared_ptr<ClientGameObj> m_gameObj_open;
};