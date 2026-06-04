#pragma once

// Central asset manifest. Single source of truth for the asset filenames that used to be scattered as
// raw string literals across the client. The *_File() lookups return the exact same filenames the code
// loaded before — this is a naming/indirection layer, not a behaviour change.
//
//   Fonts  : 100% static -> FontId is total.
//   Sfx    : the statically-named one-shot sounds. Runtime/random/DB-driven sounds (Util::randomChoice
//            variant sets, getItemSound(), zone/cast sounds, footsteps) keep using the string API.
//   Music  : only the literal-named track; data-driven zone music keeps the string API.
//   Shader : the 4 frag shaders (owned by ContentMgr).
//   Image  : raw on-disk textures NOT inside the content zip (window icon, loading splash).

#include <string>

namespace Assets
{
	// Directory prefixes (were hard-coded at the load sites).
	inline constexpr const char* kFontDir   = "content\\fonts\\";
	inline constexpr const char* kShaderDir = "scripts\\shaders\\";

	enum class FontId
	{
		Palatino,        // "Palatino Linotype Regular.ttf"
		PalatinoBold,    // "Palatino Linotype Bold.ttf"
		Helvetica,       // "Helvetica 400.ttf"
		Trebuchet,       // "trebuc.ttf"
		Cambria,         // "Cambria Regular.ttf"
		Ringbearer,      // "Ringbearer Medium.ttf"
		FrizRegular,     // "Friz Quadrata Regular.ttf"
		FrizBold,        // "Friz Quadrata Bold.ttf"
		Constantia,      // "Constantia Regular.ttf"
		Arial,           // "arial.ttf"
		Fontin,          // "Fontin-Regular.ttf"
	};

	enum class SfxId
	{
		WindowTargetOpen,     // "window_target_open_a.ogg"
		WindowTargetClose,    // "window_target_close_a.ogg"
		WindowOpen,           // "window_open_a.ogg"
		WindowClose,          // "window_close_a.ogg"
		ButtonClick,          // "button_click_a.ogg"
		ButtonChange,         // "button_change_a.ogg"
		LoginPopupOpen,       // "login_popup_open.ogg"
		BoxMessage,           // "box_message_a.ogg"
		AlertEntry,           // "alert_entry_a.ogg"
		AlertCooltimeOver,    // "alert_cooltime_over_a.ogg"
		AlertMail,            // "alert_mail_a.ogg"
		AlertMissionGet,      // "alert_mission_get_a.ogg"
		AlertMissionReward,   // "alert_mission_reward_a.ogg"
		AlertSoulstoneEmpty,  // "alert_soulstone_empty_a.ogg"
		AlertAuctionComplete, // "alert_auction_complete_a.ogg"
		AlertIncreased,       // "alert_increased_a.ogg"
		AlertLevelup,         // "alert_levelup_a.ogg"
		AlertQuestDelete,     // "alert_quest_delete_a.ogg"
		AlertQuestReward,     // "alert_quest_reward_a.ogg"
		AlertQuestGet,        // "alert_quest_get.ogg"
		AlertDeleteItem,      // "alert_delete_item_a.ogg"
		AlertScoreVictory,    // "alert_score_victory_a.ogg"
		AlertTimerFail,       // "alert_timer_fail_a.ogg"
		AlertRollover,        // "alert_rollover_a.ogg"
		AlertDp50,            // "alert_dp50_a.ogg"
		AlertParty,           // "alert_party_a.ogg"
		AlertHelpIndicator,   // "alert_help_indicator_a.ogg"
		AlertExchangeDelete,  // "alert_exchange_delete_a.ogg"
		AlertTradeConfirm,    // "alert_trade_confirm_a.ogg"
		ChunCommandCenterEdit,// "chun_command_center_edit.ogg"
		Teleport,             // "teleport.wav"
		SysGatheringSuccess,  // "sys_gathering_success.ogg"
		StartArena,           // "start_arena.ogg"
		EnterArena,           // "enter_arena.ogg"
		SocialCritical2,      // "social_critical2_a.ogg"
		SocialCombo,          // "social_combo_a.ogg"
		BlockMetalVar01,      // "block_metal_var01.wav"
		SkillDarkballAmmo,    // "m_skill_darkball_ammo.wav"
		NewZone,              // "new_zone.ogg"
		PowerZone,            // "power_zone_a.ogg"
		ItemGenMove,          // "item_gen_move.ogg"
	};

	enum class MusicId
	{
		AionCreation,    // "aion_creation.ogg"
	};

	enum class ShaderId
	{
		UnitBright,      // "unitbright.frag"
		UnitRecolor,     // "unitrecolor.frag"
		BrightContrast,  // "brightcontrast.frag"
		Minimap,         // "minimap.frag"
	};

	enum class ImageId
	{
		Icon,            // "content\\icon.png"   (window icon)
		LoadingSplash,   // "content\\loading.png"
	};

	// Filename lookups. Each returns the exact string the code used before.
	const std::string& fontFile(FontId id);     // bare ttf name (ContentMgr::getFont prepends kFontDir)
	const std::string& sfxFile(SfxId id);
	const std::string& musicFile(MusicId id);
	const std::string& shaderFile(ShaderId id); // bare frag name (ContentMgr prepends kShaderDir)
	const std::string& imageFile(ImageId id);   // full path incl. "content\\"
}

using namespace Assets;
