#include "stdafx.h"
#include "Assets.h"

// Filename tables. The strings here are the verbatim filenames the client loaded before centralisation —
// do not "tidy" them (they must match the actual asset files on disk / in the content zip).

namespace Assets
{
	const std::string& fontFile(FontId id)
	{
		static const std::string kPalatino     = "Palatino Linotype Regular.ttf";
		static const std::string kPalatinoBold  = "Palatino Linotype Bold.ttf";
		static const std::string kHelvetica     = "Helvetica 400.ttf";
		static const std::string kTrebuchet     = "trebuc.ttf";
		static const std::string kCambria       = "Cambria Regular.ttf";
		static const std::string kRingbearer    = "Ringbearer Medium.ttf";
		static const std::string kFrizRegular   = "Friz Quadrata Regular.ttf";
		static const std::string kFrizBold      = "Friz Quadrata Bold.ttf";
		static const std::string kConstantia    = "Constantia Regular.ttf";
		static const std::string kArial         = "arial.ttf";
		static const std::string kFontin        = "Fontin-Regular.ttf";

		switch (id)
		{
			case FontId::Palatino:     return kPalatino;
			case FontId::PalatinoBold: return kPalatinoBold;
			case FontId::Helvetica:    return kHelvetica;
			case FontId::Trebuchet:    return kTrebuchet;
			case FontId::Cambria:      return kCambria;
			case FontId::Ringbearer:   return kRingbearer;
			case FontId::FrizRegular:  return kFrizRegular;
			case FontId::FrizBold:     return kFrizBold;
			case FontId::Constantia:   return kConstantia;
			case FontId::Arial:        return kArial;
			case FontId::Fontin:       return kFontin;
		}

		ASSERT(false);  // unmapped FontId
		return kArial;
	}

	const std::string& sfxFile(SfxId id)
	{
		static const std::string kWindowTargetOpen      = "window_target_open_a.ogg";
		static const std::string kWindowTargetClose     = "window_target_close_a.ogg";
		static const std::string kWindowOpen            = "window_open_a.ogg";
		static const std::string kWindowClose           = "window_close_a.ogg";
		static const std::string kButtonClick           = "button_click_a.ogg";
		static const std::string kButtonChange          = "button_change_a.ogg";
		static const std::string kLoginPopupOpen        = "login_popup_open.ogg";
		static const std::string kBoxMessage            = "box_message_a.ogg";
		static const std::string kAlertEntry            = "alert_entry_a.ogg";
		static const std::string kAlertCooltimeOver     = "alert_cooltime_over_a.ogg";
		static const std::string kAlertMail             = "alert_mail_a.ogg";
		static const std::string kAlertMissionGet       = "alert_mission_get_a.ogg";
		static const std::string kAlertMissionReward    = "alert_mission_reward_a.ogg";
		static const std::string kAlertSoulstoneEmpty   = "alert_soulstone_empty_a.ogg";
		static const std::string kAlertAuctionComplete  = "alert_auction_complete_a.ogg";
		static const std::string kAlertIncreased        = "alert_increased_a.ogg";
		static const std::string kAlertLevelup          = "alert_levelup_a.ogg";
		static const std::string kAlertQuestDelete      = "alert_quest_delete_a.ogg";
		static const std::string kAlertQuestReward      = "alert_quest_reward_a.ogg";
		static const std::string kAlertQuestGet         = "alert_quest_get.ogg";
		static const std::string kAlertDeleteItem       = "alert_delete_item_a.ogg";
		static const std::string kAlertScoreVictory     = "alert_score_victory_a.ogg";
		static const std::string kAlertTimerFail        = "alert_timer_fail_a.ogg";
		static const std::string kAlertRollover         = "alert_rollover_a.ogg";
		static const std::string kAlertDp50             = "alert_dp50_a.ogg";
		static const std::string kAlertParty            = "alert_party_a.ogg";
		static const std::string kAlertHelpIndicator    = "alert_help_indicator_a.ogg";
		static const std::string kAlertExchangeDelete   = "alert_exchange_delete_a.ogg";
		static const std::string kAlertTradeConfirm     = "alert_trade_confirm_a.ogg";
		static const std::string kChunCommandCenterEdit = "chun_command_center_edit.ogg";
		static const std::string kTeleport              = "teleport.wav";
		static const std::string kSysGatheringSuccess   = "sys_gathering_success.ogg";
		static const std::string kStartArena            = "start_arena.ogg";
		static const std::string kEnterArena            = "enter_arena.ogg";
		static const std::string kSocialCritical2       = "social_critical2_a.ogg";
		static const std::string kSocialCombo           = "social_combo_a.ogg";
		static const std::string kBlockMetalVar01       = "block_metal_var01.wav";
		static const std::string kSkillDarkballAmmo     = "m_skill_darkball_ammo.wav";
		static const std::string kNewZone               = "new_zone.ogg";
		static const std::string kPowerZone             = "power_zone_a.ogg";
		static const std::string kItemGenMove           = "item_gen_move.ogg";

		switch (id)
		{
			case SfxId::WindowTargetOpen:      return kWindowTargetOpen;
			case SfxId::WindowTargetClose:     return kWindowTargetClose;
			case SfxId::WindowOpen:            return kWindowOpen;
			case SfxId::WindowClose:           return kWindowClose;
			case SfxId::ButtonClick:           return kButtonClick;
			case SfxId::ButtonChange:          return kButtonChange;
			case SfxId::LoginPopupOpen:        return kLoginPopupOpen;
			case SfxId::BoxMessage:            return kBoxMessage;
			case SfxId::AlertEntry:            return kAlertEntry;
			case SfxId::AlertCooltimeOver:     return kAlertCooltimeOver;
			case SfxId::AlertMail:             return kAlertMail;
			case SfxId::AlertMissionGet:       return kAlertMissionGet;
			case SfxId::AlertMissionReward:    return kAlertMissionReward;
			case SfxId::AlertSoulstoneEmpty:   return kAlertSoulstoneEmpty;
			case SfxId::AlertAuctionComplete:  return kAlertAuctionComplete;
			case SfxId::AlertIncreased:        return kAlertIncreased;
			case SfxId::AlertLevelup:          return kAlertLevelup;
			case SfxId::AlertQuestDelete:      return kAlertQuestDelete;
			case SfxId::AlertQuestReward:      return kAlertQuestReward;
			case SfxId::AlertQuestGet:         return kAlertQuestGet;
			case SfxId::AlertDeleteItem:       return kAlertDeleteItem;
			case SfxId::AlertScoreVictory:     return kAlertScoreVictory;
			case SfxId::AlertTimerFail:        return kAlertTimerFail;
			case SfxId::AlertRollover:         return kAlertRollover;
			case SfxId::AlertDp50:             return kAlertDp50;
			case SfxId::AlertParty:            return kAlertParty;
			case SfxId::AlertHelpIndicator:    return kAlertHelpIndicator;
			case SfxId::AlertExchangeDelete:   return kAlertExchangeDelete;
			case SfxId::AlertTradeConfirm:     return kAlertTradeConfirm;
			case SfxId::ChunCommandCenterEdit: return kChunCommandCenterEdit;
			case SfxId::Teleport:              return kTeleport;
			case SfxId::SysGatheringSuccess:   return kSysGatheringSuccess;
			case SfxId::StartArena:            return kStartArena;
			case SfxId::EnterArena:            return kEnterArena;
			case SfxId::SocialCritical2:       return kSocialCritical2;
			case SfxId::SocialCombo:           return kSocialCombo;
			case SfxId::BlockMetalVar01:       return kBlockMetalVar01;
			case SfxId::SkillDarkballAmmo:     return kSkillDarkballAmmo;
			case SfxId::NewZone:               return kNewZone;
			case SfxId::PowerZone:             return kPowerZone;
			case SfxId::ItemGenMove:           return kItemGenMove;
		}

		ASSERT(false);  // unmapped SfxId
		return kButtonClick;
	}

	const std::string& musicFile(MusicId id)
	{
		static const std::string kAionCreation = "aion_creation.ogg";

		switch (id)
		{
			case MusicId::AionCreation: return kAionCreation;
		}

		ASSERT(false);  // unmapped MusicId
		return kAionCreation;
	}

	const std::string& shaderFile(ShaderId id)
	{
		static const std::string kUnitBright     = "unitbright.frag";
		static const std::string kUnitRecolor    = "unitrecolor.frag";
		static const std::string kBrightContrast = "brightcontrast.frag";
		static const std::string kMinimap        = "minimap.frag";

		switch (id)
		{
			case ShaderId::UnitBright:     return kUnitBright;
			case ShaderId::UnitRecolor:    return kUnitRecolor;
			case ShaderId::BrightContrast: return kBrightContrast;
			case ShaderId::Minimap:        return kMinimap;
		}

		ASSERT(false);  // unmapped ShaderId
		return kUnitBright;
	}

	const std::string& imageFile(ImageId id)
	{
		static const std::string kIcon          = "content\\icon.png";
		static const std::string kLoadingSplash = "content\\loading.png";

		switch (id)
		{
			case ImageId::Icon:          return kIcon;
			case ImageId::LoadingSplash: return kLoadingSplash;
		}

		ASSERT(false);  // unmapped ImageId
		return kIcon;
	}
}
