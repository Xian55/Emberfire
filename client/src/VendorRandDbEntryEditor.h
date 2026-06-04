#pragma once

#include "TableTemplateEditor.h"

class TextBoxRo;
class PromptBox;
class Sprite;
class ClientMap;
class ClientNpc;

class VendorRandDbEntryEditor :	public TableTemplateEditor
{
	enum Interface
	{
	};

	enum Field
	{
		VRTF_entry,
		VRTF_min_level,
		VRTF_max_level,
		VRTF_equip_type,
		VRTF_weapon_type,
		VRTF_armor_preference,
		VRTF_green_chance,
		VRTF_blue_chance,
		VRTF_gold_chance,
		VRTF_purple_chance,

		NumFields,
	};

	struct FieldDisplay
	{
		shared_ptr<TextBoxRo> label;
		shared_ptr<PromptBox> prompt;
	};

	public:
		VendorRandDbEntryEditor(RenderObjectHolder& owner, const int id);
		virtual ~VendorRandDbEntryEditor();
		
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
		
		map<string, string> m_strEquipTypes;
		map<string, string> m_strWeaponTypes;
		map<string, string> m_strArmorStyles;
};