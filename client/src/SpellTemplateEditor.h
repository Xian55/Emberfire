#pragma once

#include "TableTemplateEditor.h"

#include "..\Shared\SpellDefines.h"

class Sprite;
class SpellIcon;
class ClientMap;
class ClientPlayer;
class ClientNpc;
class WorldSpellAnimation;

class SpellTemplateEditor :	public TableTemplateEditor
{
	enum Interface
	{
		SpellEntryIcon = 1,
	};

	enum Field
	{
		STF_entry,
		STF_name,
		STF_icon,
		STF_description,
		STF_aura_description,
		STF_mana_formula,
		STF_mana_pct,
		STF_health_cost,
		STF_health_pct_cost,
		STF_cast_time,
		STF_cooldown,
		STF_cooldown_category,
		STF_duration,
		STF_duration_formula,
		STF_interval,
		STF_attributes,
		STF_aura_interrupt_flags,
		STF_cast_interrupt_flags,
		STF_dispel,
		STF_maxTargets,
		STF_prevention_type,
		STF_cast_school,
		STF_magic_roll_school,
		STF_range,
		STF_range_min,
		STF_speed,
		STF_stack_amount,
		STF_activated_by_in,
		STF_activated_by_out,
		STF_required_equipslot,
		STF_req_caster_mechanic,
		STF_req_tar_mechanic,
		STF_req_caster_aura,
		STF_req_tar_aura,
		STF_abilities_tab,
		STF_can_level_up,
		STF_stat_scale_1,
		STF_stat_scale_2,

		EndFields_Main,

		//
		STF_effect1,
		STF_effect1_data1,
		STF_effect1_data2,
		STF_effect1_data3,
		STF_effect1_targetType,
		STF_effect1_radius,
		STF_effect1_positive,
		STF_effect1_scale_formula,
		
		EndFields_Effect1,
		
		//
		STF_effect2,
		STF_effect2_data1,
		STF_effect2_data2,
		STF_effect2_data3,
		STF_effect2_targetType,
		STF_effect2_radius,
		STF_effect2_positive,
		STF_effect2_scale_formula,

		EndFields_Effect2,
		
		//
		STF_effect3,
		STF_effect3_data1,
		STF_effect3_data2,
		STF_effect3_data3,
		STF_effect3_targetType,
		STF_effect3_radius,
		STF_effect3_positive,
		STF_effect3_scale_formula,

		EndFields_Effect3,

		//
		SVF_traveling_kit,
		SVF_impact_kit,
		SVF_casting_kit,
		SVF_go_kit,
		SVF_aura_kit_ontop,
		SVF_aura_kit_below,
		SVF_unit_go_animation,
		SVF_unit_cast_animation,
		SVF_uca_speed,
		SVF_uca_freeze_frame,

		EndSpellVisualFields,

		SVKF_entry,
		SVKF_psystem,
		SVKF_psystem_x,
		SVKF_psystem_y,
		SVKF_spranim,
		SVKF_spranim_x,
		SVKF_spranim_y,
		SVKF_sprcolor,
		SVKF_sprblend,
		SVKF_spranim_topmost,
		SVKF_spranim_2,
		SVKF_spranim_x_2,
		SVKF_spranim_y_2,
		SVKF_sprcolor_2,
		SVKF_sprblend_2,
		SVKF_spranim2_topmost,
		SVKF_sound,
		SVKF_directional_spranim,
		SVKF_directional_spranim_x,
		SVKF_directional_spranim_y,
		SVKF_ground_glow_color,
		SVKF_unit_glow_color,

		EndSpellVisualKitFields,

		//
		NumFields,
	};

	public:
		SpellTemplateEditor(RenderObjectHolder& owner, const int id);
		virtual ~SpellTemplateEditor();
		
	private:
		void input() final;
		void render() final;
		void initDictionary() final;
		void setEntry(const int entry) final;
		
		bool fieldVisible(const int field) const final;
			
		string fieldKeyName(const int field) const final;
		string fieldTable(const int field) const final;
		string fieldName(const int field) const final;
		string fieldValueToHumandReadable(const int field, string value) const final;
		string fieldDbName(const int field) const final;
		string huamnReadableToFieldValue(const int field, const string& readable) const final;

		int fieldEntry(const int field) const final;

	private:
		void initEffectDictionary(const int eff, const int data1, const int data2, const int data3);

		SpellDefines::Effects getSpellEffectFromName(const string& str) const;		
		SpellDefines::AuraType getAuraTypeFromName(const string& str) const;
		
		map<string, string> m_strSpriteSheets;
		map<string, string> m_strPsystems;
		map<string, string> m_strProcFlags;
		map<string, string> m_strProcType;
		map<string, string> m_strGems;
		map<string, string> m_strQualities;
		map<string, string> m_strEquipTypes;

		// Used for demonstrating spell visuals
		unique_ptr<ClientMap> m_map;
		shared_ptr<ClientNpc> m_unitVictim;
		shared_ptr<ClientNpc> m_unitCaster;
		shared_ptr<WorldSpellAnimation> m_animCasting;
		shared_ptr<WorldSpellAnimation> m_animGo;
};