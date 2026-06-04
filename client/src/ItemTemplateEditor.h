#pragma once

#include "TableTemplateEditor.h"

class TextBoxRo;
class PromptBox;
class Sprite;
class ClientMap;
class ItemIcon;

class ItemTemplateEditor :	public TableTemplateEditor
{
	enum Interface
	{
		ItemEntryIcon = 1,
	};

	enum Field
	{
		ITF_entry,
		ITF_name, 
		ITF_description, 
		ITF_icon, 
		ITF_icon_sound, 
		ITF_model, 
		ITF_flags, 
		ITF_required_level, 
		ITF_weapon_type,
		ITF_weapon_material,
		ITF_armor_type, 
		ITF_equip_type, 
		ITF_quality, 
		ITF_item_level, 
		ITF_durability, 
		ITF_sell_price, 
		ITF_stack_count,
		ITF_num_sockets,
		ITF_required_class,
		ITF_quest_offer,
		ITF_spell_1, 
		ITF_spell_2, 
		ITF_spell_3, 
		ITF_spell_4, 
		ITF_spell_5,
		ITF_generated,
		ITF_buy_cost_ratio,
		
		ITF_MainFields_End,
		
		ITF_stat_type1, 
		ITF_stat_value1,
		ITF_stat_type2, 
		ITF_stat_value2,
		ITF_stat_type3, 
		ITF_stat_value3,
		ITF_stat_type4, 
		ITF_stat_value4,
		ITF_stat_type5, 
		ITF_stat_value5,
		ITF_stat_type6, 
		ITF_stat_value6,
		ITF_stat_type7, 
		ITF_stat_value7,
		ITF_stat_type8, 
		ITF_stat_value8,
		ITF_stat_type9, 
		ITF_stat_value9,
		ITF_stat_type10,
		ITF_stat_value10,

		NumFields,
	};

	struct FieldDisplay
	{
		shared_ptr<TextBoxRo> label;
		shared_ptr<PromptBox> prompt;
	};

	public:
		ItemTemplateEditor(RenderObjectHolder& owner, const int id);
		virtual ~ItemTemplateEditor();
		
	private:
		void input() final;
		void render() final;
		void notifyCtxMenuClicked(const int id, const string& lineClicked) final;
		
		void initDictionary() override;

		int fieldEntry(const int field) const final;

		string fieldTable(const int field) const final;
		string fieldName(const int field) const final;
		string fieldDbName(const int field) const final;
		string fieldKeyName(const int field) const final;
		
		string fieldValueToHumandReadable(const int field, string value) const final;
		string huamnReadableToFieldValue(const int field, const string& readable) const final;

	private:
		void setFieldValue(const int field, string str) final;
		void setEntry(const int spellId);
		
		map<string, string> m_strSpellEntries;
		map<string, string> m_strWeaponTypes;
		map<string, string> m_strQualities;
		map<string, string> m_strArmorType;
		map<string, string> m_strEquipTypes;
		map<string, string> m_strIcons;
		map<string, string> m_strGearModels;
		map<string, string> m_strClasses;
		map<string, string> m_strWeaponMaterial;
		map<string, string> m_strItemFlags;
		map<string, string> m_strQuestEntries;

		shared_ptr<ItemIcon> m_itemIcon;

		// Used for demonstrating spell visuals
		unique_ptr<ClientMap> m_map;
};