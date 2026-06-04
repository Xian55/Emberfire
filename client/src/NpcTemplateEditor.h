#pragma once

#include "TableTemplateEditor.h"

class TextBoxRo;
class PromptBox;
class Sprite;
class ClientMap;
class ClientNpc;

class NpcTemplateEditor :	public TableTemplateEditor
{
	enum Interface
	{
	};

	enum Field
	{
		NTF_entry,
		NTF_name,
		NTF_subname,
		NTF_model_id,
		NTF_model_scale,
		NTF_portrait,
		NTF_min_level,
		NTF_max_level,		
		NTF_faction,
		NTF_type,
		NTF_bool_elite,
		NTF_bool_boss,
		NTF_npc_flags,
		NTF_game_flags,
		NTF_gossip_menu_id,
		NTF_movement_type,		
		NTF_path_id,
		NTF_ai_type,
		NTF_mechanic_immune_mask,
		NTF_loot_green_chance,
		NTF_loot_blue_chance,
		NTF_loot_gold_chance,
		NTF_loot_purple_chance,
		NTF_custom_loot,
		NTF_custom_gold_ratio,
		
		NTF_empty1,

		NTF_spell_primary,
		NTF_spell_1_id,
		NTF_spell_1_chance,
		NTF_spell_1_interval,
		NTF_spell_1_cooldown,
		NTF_spell_1_targetType,
		NTF_spell_2_id,
		NTF_spell_2_chance,
		NTF_spell_2_interval,
		NTF_spell_2_cooldown,
		NTF_spell_2_targetType,
		NTF_spell_3_id,
		NTF_spell_3_chance,
		NTF_spell_3_interval,
		NTF_spell_3_cooldown,
		NTF_spell_3_targetType,
		NTF_spell_4_id,
		NTF_spell_4_chance,
		NTF_spell_4_interval,
		NTF_spell_4_cooldown,
		NTF_spell_4_targetType,
		
		NTF_MainFields_End,
		
		NTF_health,
		NTF_mana,
		NTF_armor,
		NTF_weapon_value,
		NTF_melee_speed,
		NTF_melee_skill,
		NTF_ranged_weapon_value,
		NTF_ranged_speed,
		NTF_ranged_skill,
		NTF_shield_skill,
		NTF_strength,
		NTF_agility,
		NTF_intellect,
		NTF_willpower,
		NTF_courage,
		NTF_resistance_holy,
		NTF_resistance_frost,
		NTF_resistance_shadow,
		NTF_resistance_fire,

		NumFields,
	};

	struct FieldDisplay
	{
		shared_ptr<TextBoxRo> label;
		shared_ptr<PromptBox> prompt;
	};

	public:
		NpcTemplateEditor(RenderObjectHolder& owner, const int id);
		virtual ~NpcTemplateEditor();
		
	private:
		void input() final;
		void render() final;
		void notifyCtxMenuClicked(const int id, const string& lineClicked) final;
		void deleteEntry() final;

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
		
		map<string, string> m_strSpellEntries;
		map<string, string> m_strAiTargetTypes;

		// Used for demonstrating spell visuals
		unique_ptr<ClientMap> m_map;
		shared_ptr<ClientNpc> m_npc;
};