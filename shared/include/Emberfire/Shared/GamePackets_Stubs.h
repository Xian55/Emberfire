#pragma once
// =============================================================================
// GamePackets_Stubs.h
// -----------------------------------------------------------------------------
// Auto-reconstructed packet classes for the client (r1189).
// Field/member orders were harvested (read-only) from the client source:
//   - SEND-side (GP_Client_*) field order: member-assignment order at each
//     `pk.build(StlBuffer{})` call site + build/recon/SHARED_INTERFACE.md.
//   - RECV-side (GP_Server_*) member set + read order: the matching
//     Game::processPacket_Server_* handler in client/src/Game.cpp.
//
// This file defines ONLY the packets NOT already present in GamePacketServer.h
// / GamePacketClient.h. Those already done (do NOT redefine):
//   GP_Server_Validate, GP_Server_CharacterList, GP_Server_SetController,
//   GP_Server_ObjectVariable, GP_Server_ChatMsg,
//   GP_Client_Authenticate, GP_Client_EnterWorld, GP_Client_CharCreate,
//   GP_Client_RequestMove, GP_Client_RequestStop, GP_Mutual_Ping.
//
// Member types inferred per the task spec: guid=uint32_t, ids=int,
// names/text=std::string, flags=bool, coords=float.
//
// !! WIRE ORDER IS A BEST-EFFORT RECONSTRUCTION !!  The exact server byte order
// for many of these is unverified; bodies whose layout is uncertain are flagged
// with `// TODO verify`.  See the report accompanying this file for the list.
// =============================================================================

#include "GamePacketServer.h" // Opcode enum + GamePacket base + StlBuffer
#include "ItemDefines.h"      // ItemDefines::ItemDefinition (ItemDef is an alias of it)
#include "Geo2d.h"            // Geo2d::Vector2 (PktVec2 is an alias of it)
#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <set>

// ----------------------------------------------------------------------------
// Shared helper types
// ----------------------------------------------------------------------------

// Packet-carried item payload. The real type IS ItemDefines::ItemDefinition
// (in the client's ItemDefines.h). ItemDef is now a plain alias so the client's
// interchange of ItemDef / ItemDefinition (setItemDef, registerItem, etc.) type-checks.
using ItemDef = ItemDefines::ItemDefinition;

inline StlBuffer& operator<<(StlBuffer& b, const ItemDef& it) {
    b << it.m_itemId << it.m_affixId << it.m_affixScore << it.m_enchantLvl
      << it.m_durability << it.m_soulbound << it.m_gem1 << it.m_gem2
      << it.m_gem3 << it.m_gem4;
    return b;
}
inline StlBuffer& operator>>(StlBuffer& b, ItemDef& it) {
    b >> it.m_itemId >> it.m_affixId >> it.m_affixScore >> it.m_enchantLvl
      >> it.m_durability >> it.m_soulbound >> it.m_gem1 >> it.m_gem2
      >> it.m_gem3 >> it.m_gem4;
    return b;
}

// Simple 2d point used by movement splines. The client passes vector<PktVec2>
// into MutualUnit::setSpline(vector<Geo2d::Vector2>), so PktVec2 IS Geo2d::Vector2.
using PktVec2 = Geo2d::Vector2;
inline StlBuffer& operator>>(StlBuffer& b, PktVec2& v) { b >> v.x >> v.y; return b; }

// =============================================================================
//  CLIENT -> SERVER  (GP_Client_*)
//  Each writes [u16 opcode][fields...]; the length prefix is added by SfSocket.
// =============================================================================

// ---- Character management ----------------------------------------------------
struct GP_Client_CharacterList : GamePacket { // empty: opcode only
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_CharacterList); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_DeleteCharacter : GamePacket {
    std::uint32_t m_playerGuid = 0;
    // REVERTED to original u32-only (char deletion is consequential; the +trailing-string was unverified).
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_DeleteCharacter) << m_playerGuid; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Combat / casting --------------------------------------------------------
struct GP_Client_CastSpell : GamePacket {
    std::uint32_t m_targetGuid = 0; int m_spellId = 0; float m_targetPosX = 0, m_targetPosY = 0; bool m_ctm = false;
    // Server (process_Client_CastSpell, FUN_004957d0) reads spellId FIRST (proto lookup key),
    // then targetGuid, posX, posY, ctm. Order was swapped -> "bad spellProto" for every cast.
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_CastSpell) << std::uint32_t(m_spellId) << m_targetGuid << m_targetPosX << m_targetPosY << m_ctm; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_CancelCast : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_CancelCast); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_CancelBuff : GamePacket {
    int m_spellId = 0; std::uint32_t m_casterGuid = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_CancelBuff) << m_spellId << m_casterGuid; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_ReqTheoreticalSpell : GamePacket {
    int m_spellId = 0, m_level = 0;
    // REVERTED to original (unverified re-decode).
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_ReqTheoreticalSpell) << m_spellId << m_level; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_ReqAbilityList : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_ReqAbilityList); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Items / inventory -------------------------------------------------------
struct GP_Client_UseItem : GamePacket {
    // 255 (0xff) = "no target" sentinel (matches r1189 client). The unused field MUST be 0xff, not 0:
    // socketing an equipped item sends tgtEq=<slot>,tgtInv=0xff; an inventory item tgtInv=<slot>,tgtEq=0xff;
    // direct use (potion) sends both 0xff. A 0 here = "inventory slot 0" -> server sockets wrong item -> code 43.
    int m_slot = 0; ItemDef m_itemId; int m_target_InvSlot = 255; int m_target_EquipSlot = 255;
    // Wire (op11): slot:u8, itemId:u16, targetInvSlot:u8, targetEquipSlot:u8.
    StlBuffer& build(StlBuffer&& b) {
        b << std::uint16_t(Client_UseItem) << std::uint8_t(m_slot)
          << std::uint16_t(m_itemId.m_itemId)
          << std::uint8_t(m_target_InvSlot) << std::uint8_t(m_target_EquipSlot);
        m_buf = std::move(b); return m_buf;
    }
    StlBuffer m_buf;
};
struct GP_Client_EquipItem : GamePacket {
    int m_slotInv = 0, m_slotEquip = 0; ItemDef m_itemId;
    // Wire (op9, client serialize): slotInv:u8, slotEquip:u8, itemId:u16, affix:u16,
    //   affixScore:u8, gem1..4:u8, enchant:u8, soulbound:bool, durability:u8.
    StlBuffer& build(StlBuffer&& b) {
        b << std::uint16_t(Client_EquipItem)
          << std::uint8_t(m_slotInv) << std::uint8_t(m_slotEquip)
          << std::uint16_t(m_itemId.m_itemId) << std::uint16_t(m_itemId.m_affixId)
          << std::uint8_t(m_itemId.m_affixScore)
          << std::uint8_t(m_itemId.m_gem1) << std::uint8_t(m_itemId.m_gem2)
          << std::uint8_t(m_itemId.m_gem3) << std::uint8_t(m_itemId.m_gem4)
          << std::uint8_t(m_itemId.m_enchantLvl)
          << m_itemId.m_soulbound
          << std::uint8_t(m_itemId.m_durability);
        m_buf = std::move(b); return m_buf;
    }
    StlBuffer m_buf;
};
struct GP_Client_UnequipItem : GamePacket {
    int m_equipSlot = 0, m_inventoryDest = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_UnequipItem) << std::uint8_t(m_equipSlot) << std::uint8_t(m_inventoryDest); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_DestroyItem : GamePacket {
    int m_slot = 0; ItemDef m_itemId;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_DestroyItem) << m_slot << m_itemId; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_MoveItem : GamePacket {
    int m_from = 0, m_to = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_MoveItem) << std::uint8_t(m_from) << std::uint8_t(m_to); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_SortInventory : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_SortInventory); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_SplitItemStack : GamePacket {
    int m_slot = 0;
    // Wire (op66, GROUND TRUTH live capture, Alt+click stack): u8 slot. (Was int.)
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_SplitItemStack) << std::uint8_t(m_slot); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Vendor / repair ---------------------------------------------------------
struct GP_Client_SellItem : GamePacket {
    int m_slot = 0; ItemDef m_itemId;
    // Wire = op12, GROUND TRUTH (live capture, money-verified across 21 sells): u8 slot, u16 itemId.
    //   (op10 turned out to be DestroyItem; the dev's vendor-sell is op12 + just slot+itemId — 3 bytes.)
    StlBuffer& build(StlBuffer&& b) {
        b << std::uint16_t(Client_SellItem)
          << std::uint8_t(m_slot)
          << std::uint16_t(m_itemId.m_itemId);
        m_buf = std::move(b); return m_buf;
    }
    StlBuffer m_buf;
};
struct GP_Client_Buyback : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_Buyback); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_BuyVendorItem : GamePacket {
    ItemDef m_itemId; int m_count = 0;
    // Wire (client serialize FUN_0052d7d0): u16 op, u16 itemId, u16 affix, u8 affixScore, u8 ?, u32 count.
    // (Reconstructed wrote the full 40-byte ItemDef blob + int count → server mis-parsed → buy did nothing.)
    StlBuffer& build(StlBuffer&& b) {
        b << std::uint16_t(Client_BuyVendorItem)
          << std::uint16_t(m_itemId.m_itemId)
          << std::uint16_t(static_cast<std::uint16_t>(m_itemId.m_affixId))
          << std::uint8_t(static_cast<std::uint8_t>(m_itemId.m_affixScore))
          << std::uint8_t(0)
          << std::uint32_t(static_cast<std::uint32_t>(m_count));
        m_buf = std::move(b); return m_buf;
    }
    StlBuffer m_buf;
};
struct GP_Client_Repair : GamePacket {
    bool m_confirmed = false;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_Repair) << m_confirmed; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Bank --------------------------------------------------------------------
struct GP_Client_MoveInventoryToBank : GamePacket {
    int m_from = 0, m_to = 0; bool m_autoSelectForMe = false;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_MoveInventoryToBank) << m_from << m_to << m_autoSelectForMe; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_MoveBankToBank : GamePacket {
    int m_from = 0, m_to = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_MoveBankToBank) << m_from << m_to; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_UnBankItem : GamePacket {
    int m_slot = 0, m_inventorySlot = 0; bool m_chooseInvSlotForMe = false;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_UnBankItem) << m_slot << m_inventorySlot << m_chooseInvSlotForMe; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_SortBank : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_SortBank); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Looting -----------------------------------------------------------------
struct GP_Client_LootItem : GamePacket {
    static constexpr std::uint16_t TakeAll = 0xFFFF; // sentinel itemId meaning "loot everything"
    std::uint32_t m_sourceGuid = 0; ItemDef m_itemId;
    // Wire (VERIFIED, client serialize FUN_004a0820 @op30=0x1e): u16 op, u32 sourceGuid, u16 itemId,
    //   u16 affix, u8 affixScore, u8 ?. Identifies the loot by SOURCE + ITEM (not a slot index).
    //   TakeAll = itemId 0xFFFF sentinel. (Earlier u8-slot/op66 attempt was the Alt-stack variant; the
    //   normal window-click take is op30 with the item identity, matching the dev's LootWindow.cpp.)
    StlBuffer& build(StlBuffer&& b) {
        b << std::uint16_t(Client_LootItem)
          << m_sourceGuid
          << static_cast<std::uint16_t>(m_itemId.m_itemId)
          << static_cast<std::uint16_t>(m_itemId.m_affixId)
          << static_cast<std::uint8_t>(m_itemId.m_affixScore)
          << static_cast<std::uint8_t>(m_itemId.m_durability);  // 6th byte: send real durability so the
              // server's item-identity match succeeds for gear (was 0 → durable items failed to match).
        m_buf = std::move(b); return m_buf;
    }
    StlBuffer m_buf;
};

// ---- Chat / social -----------------------------------------------------------
struct GP_Client_ChatMsg : GamePacket {
    int m_channelId = 0; std::string m_text, m_targetName; ItemDef m_itemId;
    // Wire CONFIRMED from steam capture (op17, session conn2-v1189): channel:u8, text:cstr, targetName:cstr,
    // then a 12-byte item-link blob (all-zero when no item is linked). channel is u8 (was sent as int/u32 ->
    // 3 junk bytes into text). targetName is empty for non-whisper. OMITTING the 12-byte blob made the packet
    // 12 bytes short -> the server underflowed reading the link and DROPPED THE CONNECTION (every chat send).
    // The blob's internal field layout is unknown (no capture with a linked item yet), so emit 12 zero bytes;
    // linking an item into chat (shift-click) is a separate follow-up. For non-whisper channels these bytes
    // are byte-identical to the original capture.
    StlBuffer& build(StlBuffer&& b) {
        b << std::uint16_t(Client_ChatMsg) << std::uint8_t(m_channelId) << m_text << m_targetName;
        b << std::uint32_t(0) << std::uint32_t(0) << std::uint32_t(0); // 12-byte item-link blob (zeroed = no link)
        m_buf = std::move(b); return m_buf;
    }
    StlBuffer m_buf;
};
struct GP_Client_SetIgnorePlayer : GamePacket {
    bool m_ignore = false; std::string m_name;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_SetIgnorePlayer) << m_ignore << m_name; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_ReportPlayer : GamePacket {
    std::string m_name;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_ReportPlayer) << m_name; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_RollDice : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_RollDice); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_TogglePvP : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_TogglePvP); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Moderator commands (name only) -----------------------------------------
struct GP_Client_MOD_WarnPlayer : GamePacket {
    std::string m_name;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_MOD_WarnPlayer) << m_name; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_MOD_MutePlayer : GamePacket {
    std::string m_name;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_MOD_MutePlayer) << m_name; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_MOD_KickPlayer : GamePacket {
    std::string m_name;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_MOD_KickPlayer) << m_name; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_MOD_BanPlayer : GamePacket {
    std::string m_name;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_MOD_BanPlayer) << m_name; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Guild -------------------------------------------------------------------
struct GP_Client_GuildCreate : GamePacket {
    std::string m_guildName;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_GuildCreate) << m_guildName; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_GuildInviteMember : GamePacket {
    std::string m_playerName;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_GuildInviteMember) << m_playerName; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_GuildKickMember : GamePacket {
    std::uint32_t m_targetGuid = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_GuildKickMember) << m_targetGuid; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_GuildPromoteMember : GamePacket {
    std::uint32_t m_targetGuid = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_GuildPromoteMember) << m_targetGuid; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_GuildDemoteMember : GamePacket {
    std::uint32_t m_targetGuid = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_GuildDemoteMember) << m_targetGuid; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_GuildQuit : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_GuildQuit); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_GuildDisband : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_GuildDisband); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_GuildMotd : GamePacket {
    std::string m_motd;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_GuildMotd) << m_motd; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_GuildRosterRequest : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_GuildRosterRequest); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_GuildInviteResponse : GamePacket {
    int m_guildId = 0; bool m_accept = false;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_GuildInviteResponse) << m_guildId << m_accept; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Duels -------------------------------------------------------------------
struct GP_Client_YieldDuel : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_YieldDuel); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_DuelResponse : GamePacket {
    bool m_accept = false;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_DuelResponse) << m_accept; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Party -------------------------------------------------------------------
struct GP_Client_PartyInviteMember : GamePacket {
    std::string m_playerName;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_PartyInviteMember) << m_playerName; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_PartyInviteResponse : GamePacket {
    bool m_accept = false;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_PartyInviteResponse) << m_accept; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
// PartyChanges multiplexes several actions; only one field is set per send.
// Wire order is a best guess (leave flag + the three guids). // TODO verify
struct GP_Client_PartyChanges : GamePacket {
    bool m_leaveParty = false;
    std::uint32_t m_kickPlayerGuid = 0, m_promotePlayerGuid = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_PartyChanges) << std::uint8_t(m_leaveParty) << std::uint32_t(m_kickPlayerGuid) << std::uint32_t(m_promotePlayerGuid); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Trade -------------------------------------------------------------------
struct GP_Client_OpenTradeWith : GamePacket {
    std::string m_playerName;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_OpenTradeWith) << m_playerName; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_TradeAddItem : GamePacket {
    int m_invSlot = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_TradeAddItem) << m_invSlot; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_TradeRemoveItem : GamePacket {
    std::uint32_t m_itemGuid = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_TradeRemoveItem) << m_itemGuid; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_TradeSetGold : GamePacket {
    int m_amount = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_TradeSetGold) << m_amount; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_TradeConfirm : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_TradeConfirm); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_TradeCancel : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_TradeCancel); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Gossip / quests ---------------------------------------------------------
struct GP_Client_ClickedGossipOption : GamePacket {
    int m_entry = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_ClickedGossipOption) << m_entry; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_AcceptQuest : GamePacket {
    std::uint32_t m_questGiverGuid = 0; int m_questId = 0;
    // VERIFIED vs steam client (capture conn1 18-58): quest accept = op7 {questId:u32, giverGuid:u32},
    // server replies AcceptedQuest(112). op8 (the earlier guess) is a different path the server ignored
    // from the gossip flow. giverGuid = the open GossipMenu npc (server tolerates 0 too).
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_GossipQuestAccept) << m_questId << m_questGiverGuid; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_CompleteQuest : GamePacket {
    std::uint32_t m_questGiverGuid = 0; int m_questId = 0, m_itemChoiceIdx = 0;
    // Server (process op33, FUN_0048a310) reads THREE ints and uses #1 + #3 via FUN_00432e70:
    // #1 = questId (matched vs active-quest list), #2 = ignored, #3 = itemChoiceIdx. questId MUST be first.
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_CompleteQuest) << m_questId << m_questGiverGuid << m_itemChoiceIdx; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_AbandonQuest : GamePacket {
    int m_questId = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_AbandonQuest) << m_questId; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Waypoints ---------------------------------------------------------------
struct GP_Client_QueryWaypoints : GamePacket {
    std::uint32_t m_nearbyWaypointGuid = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_QueryWaypoints) << m_nearbyWaypointGuid; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_ActivateWaypoint : GamePacket {
    std::uint32_t m_nearbyWaypointGuid = 0, m_targetWaypointGuid = 0;
    // REVERTED to original order (unverified re-decode).
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_ActivateWaypoint) << m_nearbyWaypointGuid << m_targetWaypointGuid; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Selection / progression -------------------------------------------------
struct GP_Client_SetSelected : GamePacket {
    std::uint32_t m_guid = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_SetSelected) << m_guid; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
// LevelUp carries the pending spend maps. Wire layout (count + pairs each) is a
// best guess. // TODO verify
struct GP_Client_LevelUp : GamePacket {
    // Client assigns these directly from Abilities::getDesiredInvestments() (map<int,int>)
    // and Equipment::getPendingStatInvestments() (map<UnitDefines::Stat,int>).
    std::map<int, int> m_spellInvestments;
    std::map<UnitDefines::Stat, int> m_statInvestments;
    // Wire statId = the stat's enum/var index (UnitDefines::Stat): Mana=1, Health=2, ArmorValue=3,
    // Strength=4, ... Bartering=25, Maces=29. WeaponValue=0 is non-spendable. CONFIRMED: Strength->4,
    // Bartering->25, Maces->29 (Gooday conn36/37); Health->2 / Mana->1 (live original-server spend test).
    static int spendId(UnitDefines::Stat s) {
        // Wire spend-id == the stat's enum/var index for EVERY stat (Mana=1, Health=2, ArmorValue=3,
        // Strength=4, ...). An earlier version swapped Health<->Mana (Health=1/Mana=2) from a mis-read conn13
        // capture; LIVE on the original server that made an invested-Health point land on Mana — statId 1 is
        // Mana, statId 2 is Health (plain enum order). The swap is removed; do NOT reintroduce it.
        return static_cast<int>(s);
    }

    StlBuffer& build(StlBuffer&& b) {
        // GROUND TRUTH (v1189 captures: conn13 9-stat spend, conn9/12 single-stat, conn1 spell spend):
        //   op40 = [statCount:u16][statId:u16, val:u16]* [spellCount:u16][spellId:u16, val:u16]*
        //   STATS come FIRST; statId = UnitDefines::Stat index + kStatIdBase (conn13: Strength idx3 -> 4).
        //   Spells come second, raw spell id. ALL u16; only NON-ZERO investments written.
        //   (Old bug chain: u32 widths + dumping all entries -> 458B desync; then sections swapped +
        //    statId-as-var -> server threw on a non-stat id -> disconnect. Both fixed here.)
        //   NOTE: spell investments additionally need an op41 SpendExp first (conn1) — not yet wired, so
        //   ability-only spends remain TODO; STAT spends (the general/skill tabs) use op40 alone.
        b << std::uint16_t(Client_LevelUp);

        std::uint16_t statN = 0;
        for (auto& p : m_statInvestments) if (p.second != 0) ++statN;
        b << statN;
        for (auto& p : m_statInvestments)
            if (p.second != 0)
                b << std::uint16_t(spendId(p.first)) << std::uint16_t(p.second);

        std::uint16_t spellN = 0;
        for (auto& p : m_spellInvestments) if (p.second != 0) ++spellN;
        b << spellN;
        for (auto& p : m_spellInvestments)
            if (p.second != 0) b << std::uint16_t(p.first) << std::uint16_t(p.second);

        m_buf = std::move(b); return m_buf;
    }
    StlBuffer m_buf;
};
// SpendExp (op41). Server FUN_004929a0 reads {u16 spellId, u16 rank}; spellId validated (FUN_0047a120),
// rank must be 1..6. CONFIRMED layout vs ghidra + capture conn1 (`2b 00 03 00` = spell 43, rank 3).
// The original client sends one of these per desired spell rank-up BEFORE the op40 LevelUp commit; rank =
// the spell's TARGET rank (current level + points being spent). Wired in World::requestLevelup().
struct GP_Client_SpellRankInvest : GamePacket {
    int m_spellId = 0, m_rank = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_SpellRankInvest) << std::uint16_t(m_spellId) << std::uint16_t(m_rank); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_RequestRespawn : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_RequestRespawn); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_Respec : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_Respec); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_ResetDungeons : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_ResetDungeons); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// ---- Arena / channels / mail -------------------------------------------------
// UpdateArenaStatus multiplexes enter/leave; one bool set per send. // TODO verify
struct GP_Client_UpdateArenaStatus : GamePacket {
    bool m_enterArena = false, m_leaveQueue = false;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_UpdateArenaStatus) << m_enterArena << m_leaveQueue; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_ChangeChannels : GamePacket {
    int m_channelId = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_ChangeChannels) << m_channelId; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_RecoverMailLoot : GamePacket { // empty
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_RecoverMailLoot); m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// =============================================================================
//  SERVER -> CLIENT  (GP_Server_*)
//  Each reads its fields from a freshly-received StlBuffer (opcode pre-stripped).
//  Member set comes from the matching processPacket_Server_* handler; the read
//  order within each is a best-effort reconstruction. // TODO verify ordering.
// =============================================================================

// ---- Login / queue -----------------------------------------------------------
struct GP_Server_QueuePosition : GamePacket {
    int m_position = 0;
    void unpack(StlBuffer& d) { d >> m_position; }
};
struct GP_Server_CharaCreateResult : GamePacket {
    std::uint8_t m_result = 0;
    void unpack(StlBuffer& d) { d >> m_result; }
};

// ---- World / objects ---------------------------------------------------------
// op77 = GP_Server_Player (the entry spawn; there is no separate NewWorld class on the wire).
// BASE object (server build FUN_00461960): guid:u32, x:f32, y:f32, varCount:u32,
//   varCount * { varId:u8, value:i32 }.
// Player extra (FUN_0046c700): orient:f32, name:cstr, guild:cstr, u8 flagA, u8 flagB, u8 flagC,
//   equipMeta:u32, then equipment items (13 bytes each, to end of payload):
//     slot:u8, itemId:u16, affix:u16, u8 b0..b5, u8 durability(b6), u8 b7.
// NOTE: op77 carries NO mapId — m_guid happens to equal the local map id (Baday guid 1 == fanadin
//   map 1), which is why setMap(m_guid) works for the single-map local setup. TODO: real map source.
struct GP_Server_NewWorld : GamePacket {
    std::uint32_t m_mapId = 0;   // actually the player guid (see note); reused as map id locally
    float m_x = 0.f, m_y = 0.f;
    std::vector<std::pair<std::uint8_t, std::int32_t>> m_variables; // full var table (Health/Level/stats...)
    struct Equip { std::uint8_t slot = 0; ItemDef item; };
    std::vector<Equip> m_equipment;
    std::string m_name, m_guild;
    void unpack(StlBuffer& d) {
        d >> m_mapId >> m_x >> m_y;
        std::uint32_t varCount = 0; d >> varCount;
        if (varCount > 4096) return;                 // sanity guard against a misparse
        for (std::uint32_t i = 0; i < varCount && d.size() >= 5; ++i) {
            std::uint8_t id = 0; std::int32_t val = 0; d >> id >> val;
            m_variables.emplace_back(id, val);
        }
        float orient = 0.f; d >> orient;
        d >> m_name >> m_guild;
        std::uint8_t fa = 0, fb = 0, fc = 0; std::uint32_t equipMeta = 0;
        d >> fa >> fb >> fc >> equipMeta;
        // Equipment runs to the end of the payload (13-byte records). Layout (verified vs DB):
        //   slot:u8, itemId:u16, affix:u16, affixScore:u8, gem1..4:u8, enchant:u8, soulbound:u8, durability:u8.
        while (d.size() >= 13) {
            Equip e;
            std::uint16_t itemId = 0, affix = 0;
            std::uint8_t affixScore = 0, g1 = 0, g2 = 0, g3 = 0, g4 = 0, ench = 0, soul = 0, dura = 0;
            d >> e.slot >> itemId >> affix >> affixScore >> g1 >> g2 >> g3 >> g4 >> ench >> soul >> dura;
            e.item.m_itemId = itemId;
            e.item.m_affixId = affix;
            e.item.m_affixScore = affixScore;
            e.item.m_gem1 = g1; e.item.m_gem2 = g2; e.item.m_gem3 = g3; e.item.m_gem4 = g4;
            e.item.m_enchantLvl = ench;
            e.item.m_soulbound = soul;
            e.item.m_durability = dura;
            m_equipment.push_back(e);
        }
    }
};
struct GP_Server_Player : GamePacket {
    std::uint32_t m_guid = 0;
    std::uint8_t m_classId = 0, m_gender = 0, m_portraitId = 0;
    float m_orientation = 0; std::string m_name, m_subName; float m_x = 0, m_y = 0;
    std::vector<std::pair<int, ItemDef>> m_equipment;   // (equipSlot, item)
    std::vector<std::pair<int, int>>     m_variables;   // (variableId, value)
    void unpack(StlBuffer& d) {
        d >> m_guid >> m_classId >> m_gender >> m_portraitId >> m_orientation
          >> m_name >> m_subName >> m_x >> m_y;
        std::uint32_t n = 0;
        d >> n; for (std::uint32_t i = 0; i < n; ++i) { std::pair<int, ItemDef> e; d >> e.first >> e.second; m_equipment.push_back(e); }
        d >> n; for (std::uint32_t i = 0; i < n; ++i) { std::pair<int, int> e; d >> e.first >> e.second; m_variables.push_back(e); }
    }
};
struct GP_Server_Npc : GamePacket {
    std::uint32_t m_guid = 0; float m_orientation = 0; int m_entry = 0; float m_x = 0, m_y = 0;
    std::vector<std::pair<int, int>> m_variables;
    void unpack(StlBuffer& d) {
        // Base-object layout (shared w/ NewWorld/GameObject, server FUN_00461960):
        //   guid:u32, x:f32, y:f32, varCount:u32, varCount x {u8 id, i32 val}, orient:f32
        // then the Npc-specific trailing entry:u32 (npc_template id).
        d >> m_guid >> m_x >> m_y;
        std::uint32_t varCount = 0; d >> varCount;
        if (varCount > 4096) return;                 // sanity guard against a misparse
        for (std::uint32_t i = 0; i < varCount && d.size() >= 5; ++i) {
            std::uint8_t id = 0; std::int32_t val = 0; d >> id >> val;
            m_variables.emplace_back(id, val);
        }
        d >> m_orientation;
        d >> m_entry;
    }
};
struct GP_Server_GameObject : GamePacket {
    std::uint32_t m_guid = 0; int m_entry = 0; float m_x = 0, m_y = 0;
    std::vector<std::pair<int, int>> m_variables;
    void unpack(StlBuffer& d) {
        // Base-object (FUN_00461960): guid,x,y,varCount,vars{u8,i32}, then GameObject trailing entry:u32.
        // VERIFIED (builder FUN_004621a0): GameObject writes base + entry:u32 ONLY — NO orient float
        // (unlike Npc op78). Reading a phantom orient here shifted entry and mis-spawned game objects.
        d >> m_guid >> m_x >> m_y;
        std::uint32_t varCount = 0; d >> varCount;
        if (varCount > 4096) return;
        for (std::uint32_t i = 0; i < varCount && d.size() >= 5; ++i) {
            std::uint8_t id = 0; std::int32_t val = 0; d >> id >> val;
            m_variables.emplace_back(id, val);
        }
        d >> m_entry;
    }
};
struct GP_Server_DestroyObject : GamePacket {
    std::uint32_t m_guid = 0;
    void unpack(StlBuffer& d) { d >> m_guid; }
};
struct GP_Server_SetSubname : GamePacket {
    std::uint32_t m_objectGuid = 0; std::string m_name;
    void unpack(StlBuffer& d) { d >> m_objectGuid >> m_name; }
};

// ---- Movement / orientation --------------------------------------------------
struct GP_Server_UnitSpline : GamePacket {
    std::uint32_t m_guid = 0; float m_startX = 0, m_startY = 0; bool m_slide = false, m_silent = false;
    std::vector<PktVec2> m_spline;
    void unpack(StlBuffer& d) {
        // GROUND TRUTH (live capture, 1541 samples): guid:u32, silent:u8, startX:f32, startY:f32,
        //   slide:u8, count:u16, count×{x:f32, y:f32}. The decompile agent SWAPPED silent<->slide:
        //   the server echoes the LOCAL player's own moves as op90 with silent=1 (923/1541 are for our
        //   guid). Reading byte[4] as slide made every self-echo a sliding-spline → permanent
        //   isSlidingSpline() → canAct()=false → frozen player + greyed abilities. With silent at byte[4],
        //   self op90 hits the handler's silent-branch (ignored, we move by prediction) and NPCs
        //   (silent=0) render their splines.
        d >> m_guid >> m_silent >> m_startX >> m_startY >> m_slide;
        std::uint16_t n = 0; d >> n;
        for (std::uint16_t i = 0; i < n; ++i) { PktVec2 v; d >> v; m_spline.push_back(v); }
    }
};
struct GP_Server_UnitOrientation : GamePacket {
    std::uint32_t m_guid = 0; float m_newOri = 0;
    void unpack(StlBuffer& d) { d >> m_guid >> m_newOri; }
};
struct GP_Server_UnitTeleport : GamePacket {
    std::uint32_t m_guid = 0; float m_newX = 0, m_newY = 0, m_newOri = 0;
    void unpack(StlBuffer& d) { d >> m_guid >> m_newX >> m_newY >> m_newOri; }
};

// ---- Inventory / bank / equipment -------------------------------------------
struct GP_Server_Inventory : GamePacket {
    struct Slot { std::uint16_t slot = 0; ItemDef itemId; int stackCount = 0; };
    std::vector<Slot> m_slots;
    // Wire (op85). count:u8 = number of inventory slots (ALL slots sent dense, empty=entry 0,
    // slot=index). Per slot 13 bytes, VERIFIED by raw-byte dump vs DB (count is the LAST byte):
    //   entry:u16, affix:u16, affixScore:u8, gem1..4:u8, enchant:u8, soulbound:u8, durability:u8, count:u8.
    void unpack(StlBuffer& d) {
        std::uint8_t n = 0; d >> n;
        for (std::uint8_t i = 0; i < n; ++i) {
            Slot s; s.slot = i;
            std::uint16_t entry = 0, affix = 0;
            std::uint8_t affixScore = 0, g1 = 0, g2 = 0, g3 = 0, g4 = 0, ench = 0, soul = 0, dura = 0, count = 0;
            d >> entry >> affix >> affixScore >> g1 >> g2 >> g3 >> g4 >> ench >> soul >> dura >> count;
            s.itemId.m_itemId = entry;
            s.itemId.m_affixId = affix;
            s.itemId.m_affixScore = affixScore;
            s.itemId.m_gem1 = g1; s.itemId.m_gem2 = g2; s.itemId.m_gem3 = g3; s.itemId.m_gem4 = g4;
            s.itemId.m_enchantLvl = ench;
            s.itemId.m_soulbound = soul;
            s.itemId.m_durability = dura;
            s.stackCount = count;
            m_slots.push_back(s);
        }
    }
};
struct GP_Server_Bank : GamePacket {
    struct Slot { std::uint16_t slot = 0; ItemDef itemId; int stackCount = 0; };
    std::vector<Slot> m_slots;
    // Same item-record format as op85 Inventory (count:u8, dense slots, 13B each).
    void unpack(StlBuffer& d) {
        std::uint8_t n = 0; d >> n;
        for (std::uint8_t i = 0; i < n; ++i) {
            Slot s; s.slot = i;
            std::uint16_t entry = 0, affix = 0;
            std::uint8_t affixScore = 0, g1 = 0, g2 = 0, g3 = 0, g4 = 0, ench = 0, soul = 0, dura = 0, count = 0;
            d >> entry >> affix >> affixScore >> g1 >> g2 >> g3 >> g4 >> ench >> soul >> dura >> count;
            s.itemId.m_itemId = entry; s.itemId.m_affixId = affix; s.itemId.m_affixScore = affixScore;
            s.itemId.m_gem1 = g1; s.itemId.m_gem2 = g2; s.itemId.m_gem3 = g3; s.itemId.m_gem4 = g4;
            s.itemId.m_enchantLvl = ench; s.itemId.m_soulbound = soul; s.itemId.m_durability = dura;
            s.stackCount = count;
            m_slots.push_back(s);
        }
    }
};
struct GP_Server_EquipItem : GamePacket {
    std::uint32_t m_guid = 0; int m_slot = 0; ItemDef m_itemId; bool m_silent = false;
    // Wire (op86, server build FUN_004768f0): guid:u32, slot:u8, itemId:u16, affix:u16,
    //   affixScore:u8, gem1..4:u8, enchant:u8, soulbound:u8, durability:u8 (18 bytes). (A trailing
    //   silent-flag byte MAY exist — reverted that +read; reading it when absent would throw and hide
    //   the live equip update. Leftover bytes are harmless.)
    void unpack(StlBuffer& d) {
        std::uint8_t slot = 0;
        std::uint16_t itemId = 0, affix = 0;
        std::uint8_t affixScore = 0, g1 = 0, g2 = 0, g3 = 0, g4 = 0, ench = 0, soul = 0, dura = 0;
        d >> m_guid >> slot >> itemId >> affix >> affixScore >> g1 >> g2 >> g3 >> g4 >> ench >> soul >> dura;
        m_slot = slot;
        m_itemId.m_itemId = itemId; m_itemId.m_affixId = affix; m_itemId.m_affixScore = affixScore;
        m_itemId.m_gem1 = g1; m_itemId.m_gem2 = g2; m_itemId.m_gem3 = g3; m_itemId.m_gem4 = g4;
        m_itemId.m_enchantLvl = ench; m_itemId.m_soulbound = soul; m_itemId.m_durability = dura;
        m_silent = false;
    }
};

// ---- Loot --------------------------------------------------------------------
struct GP_Server_OpenLootWindow : GamePacket {
    std::uint32_t m_objGuid = 0; int m_money = 0;
    std::vector<std::pair<ItemDef, int>> m_items; // (item, count)
    // Wire (op104, live capture): objGuid:u32, money:u32, itemCount:u32, items×7B
    //   {itemId:u16, affix:u16, affixScore:u8, durability:u8, count:u8}.
    //   (Reconstructed read the full 35B ItemDef via `>> e.first` per 7B item → underflow → threw →
    //    loot window never opened.)
    void unpack(StlBuffer& d) {
        d >> m_objGuid >> m_money;
        std::uint32_t n = 0; d >> n;
        for (std::uint32_t i = 0; i < n; ++i) {
            std::uint16_t itemId = 0, affix = 0; std::uint8_t affixScore = 0, dura = 0, count = 0;
            d >> itemId >> affix >> affixScore >> dura >> count;
            ItemDef def;
            def.m_itemId = itemId; def.m_affixId = affix; def.m_affixScore = affixScore; def.m_durability = dura;
            m_items.push_back({ def, static_cast<int>(count) });
        }
    }
};
struct GP_Server_NotifyItemAdd : GamePacket {
    ItemDef m_itemId; int m_amount = 0; std::string m_looterName;
    // GROUND TRUTH (live capture, 56 samples, 16B self-loots): affixId:u16, itemId:u16, 7×u8
    //   (affixScore,gem1..4,enchant,durability), amount:u32, looterName:cstr. Verified: gold = itemId 24
    //   at bytes[2-3], amounts 6/62/1 at the u32. The decompile agent SWAPPED itemId<->affixId (read
    //   itemId from the affix field → bad item identity → crashed formItemTitle on TAKE ALL).
    void unpack(StlBuffer& d) {
        std::uint16_t affix = 0, itemId = 0;
        std::uint8_t affixScore = 0, g1 = 0, g2 = 0, g3 = 0, g4 = 0, ench = 0, dura = 0;
        std::uint32_t amount = 0;
        d >> affix >> itemId >> affixScore >> g1 >> g2 >> g3 >> g4 >> ench >> dura >> amount >> m_looterName;
        m_itemId.m_itemId = itemId; m_itemId.m_affixId = affix; m_itemId.m_affixScore = affixScore;
        m_itemId.m_gem1 = g1; m_itemId.m_gem2 = g2; m_itemId.m_gem3 = g3; m_itemId.m_gem4 = g4;
        m_itemId.m_enchantLvl = ench; m_itemId.m_durability = dura;
        m_amount = static_cast<int>(amount);
    }
};
struct GP_Server_OnObjectWasLooted : GamePacket {
    std::uint32_t m_guid = 0, m_looterGuid = 0;
    void unpack(StlBuffer& d) { d >> m_guid >> m_looterGuid; }
};

// ---- Casting / combat --------------------------------------------------------
struct GP_Server_CastStart : GamePacket {
    std::uint32_t m_guid = 0; int m_spellId = 0, m_timer = 0;
    // Wire (verified): guid:u32, spellId:u16, timer:u32.
    void unpack(StlBuffer& d) { std::uint16_t s = 0; d >> m_guid >> s >> m_timer; m_spellId = s; }
};
struct GP_Server_CastStop : GamePacket {
    std::uint32_t m_guid = 0; int m_spellId = 0;
    // Wire: guid:u32, spellId:u16.
    void unpack(StlBuffer& d) { std::uint16_t s = 0; d >> m_guid >> s; m_spellId = s; }
};
struct GP_Server_SpellGo : GamePacket {
    std::uint32_t m_casterGuid = 0; int m_spellId = 0; float m_groundX = 0, m_groundY = 0;
    std::vector<std::pair<std::uint32_t, int>> m_targets; // (targetGuid, data)
    void unpack(StlBuffer& d) {
        // Real layout (verified vs live hexdump): caster:u32, spellId:u32, count:u16,
        // count x {targetGuid:u32, data:u32}, groundX:f32, groundY:f32.
        d >> m_casterGuid >> m_spellId;
        std::uint16_t n = 0; d >> n;
        if (n > 256) return;
        for (std::uint16_t i = 0; i < n; ++i) { std::uint32_t g = 0, dt = 0; d >> g >> dt; m_targets.emplace_back(g, (int)dt); }
        d >> m_groundX >> m_groundY;
    }
};
struct GP_Server_CombatMsg : GamePacket {
    std::uint32_t m_casterGuid = 0, m_targetGuid = 0;
    int m_spellId = 0, m_spellEffect = 0, m_spellResult = 0, m_auraEffect = 0, m_amount = 0;
    bool m_periodic = false, m_positive = false;
    void unpack(StlBuffer& d) {
        // Real 17-byte layout (server serializer FUN_00411a00), verified vs live hexdump:
        //   caster:u32, target:u32, hitResult:u8, amount:i16, spellId:u16,
        //   effect:u8, periodic:u8(bool), auraEffect:u8, positive:u8(bool).
        std::uint8_t result = 0, eff = 0, periodic = 0, aura = 0, positive = 0;
        std::int16_t amount = 0; std::uint16_t spellId = 0;
        d >> m_casterGuid >> m_targetGuid >> result >> amount >> spellId
          >> eff >> periodic >> aura >> positive;
        m_spellResult = result;
        m_spellId     = spellId;
        m_spellEffect = eff;
        m_auraEffect  = aura;
        m_periodic    = (periodic != 0);
        m_positive    = (positive != 0);
        // amount is already signed on the wire: negative == damage, positive == heal/restore.
        m_amount      = amount;
    }
};

// ---- Cooldowns / auras / spellbook ------------------------------------------
struct GP_Server_Cooldown : GamePacket {
    int m_id = 0, m_totalDuration = 0, m_elapsed = 0;
    // GROUND TRUTH (live capture, 12B, 3×u32): id:u32 (spellId), totalDuration:u32 (ms), elapsed:u32 (=0
    //   on a fresh cooldown). Verified: id=4(Charge)/81(Melee Swing), total=24000/2000ms. (The earlier
    //   grey-abilities regression was op90, not this — the 3rd u32 is correct.)
    void unpack(StlBuffer& d) { d >> m_id >> m_totalDuration >> m_elapsed; }
};
struct GP_Server_UnitAuras : GamePacket {
    struct AuraInfo {
        int spellId = 0; int stackCount = 0; std::uint32_t casterGuid = 0;
        int maxDuration = 0, elapsedTime = 0;
        // Filled client-side after receipt (not on the wire):
        long long startDate = 0, endDate = 0;
    };
    std::uint32_t m_unitGuid = 0;
    std::vector<AuraInfo> m_buffs, m_debuffs;
    // Real per-aura layout (verified vs live hexdump):
    //   spellId:u16, maxDuration:u32, elapsedTime:u32, stackCount:u8, casterGuid:u32  (15 bytes).
    // List count is a u8. Misreading it (u32 + wrong field order) produced garbage spellIds
    // -> random Model/scale auras applied to units.
    static void readList(StlBuffer& d, std::vector<AuraInfo>& out) {
        std::uint8_t n = 0; d >> n;
        for (std::uint8_t i = 0; i < n; ++i) {
            AuraInfo a;
            std::uint16_t spellId = 0; std::uint32_t maxDur = 0, elapsed = 0, caster = 0; std::uint8_t stack = 0;
            d >> spellId >> maxDur >> elapsed >> stack >> caster;
            a.spellId = spellId; a.maxDuration = int(maxDur); a.elapsedTime = int(elapsed);
            a.stackCount = stack; a.casterGuid = caster;
            out.push_back(a);
        }
    }
    void unpack(StlBuffer& d) { d >> m_unitGuid; readList(d, m_buffs); readList(d, m_debuffs); }
};
struct GP_Server_Spellbook : GamePacket {
    struct SpellSlot {
        int spellId = 0; std::uint8_t level = 0;
        std::vector<std::pair<std::int16_t, std::int16_t>> bpoints; // base-point ranges
    };
    std::vector<SpellSlot> m_slots;
    // Wire (op109, server build FUN_0046dd70, verified by raw dump): count:u8, per spell:
    //   spellId:u16, level:u8, subCount:u8, subCount × { i16, i16 } (base-point ranges per rank).
    void unpack(StlBuffer& d) {
        std::uint8_t n = 0; d >> n;
        for (std::uint8_t i = 0; i < n; ++i) {
            SpellSlot s;
            std::uint16_t spellId = 0; std::uint8_t level = 0, subCount = 0;
            d >> spellId >> level >> subCount;
            s.spellId = spellId;
            s.level = level;
            for (std::uint8_t j = 0; j < subCount; ++j) {
                std::int16_t a = 0, b = 0; d >> a >> b;
                s.bpoints.emplace_back(a, b);
            }
            m_slots.push_back(s);
        }
    }
};
struct GP_Server_Spellbook_Update : GamePacket {
    int m_spellId = 0; std::uint8_t m_level = 0;
    std::vector<std::pair<std::int16_t, std::int16_t>> m_bpoints;
    // Single spell entry, same format as one op109 spell: spellId:u16, level:u8, subCount:u8, subCount×{i16,i16}.
    void unpack(StlBuffer& d) {
        std::uint16_t spellId = 0; std::uint8_t subCount = 0;
        d >> spellId >> m_level >> subCount;
        m_spellId = spellId;
        for (std::uint8_t i = 0; i < subCount; ++i) { std::int16_t a = 0, b = 0; d >> a >> b; m_bpoints.emplace_back(a, b); }
    }
};
struct GP_Server_LearnedSpell : GamePacket {
    int m_spellId = 0;
    void unpack(StlBuffer& d) { d >> m_spellId; }
};

// ---- Gossip / vendor ---------------------------------------------------------
struct GP_Server_GossipMenu : GamePacket {
    struct VendorSlot { ItemDef m_itemId; int m_cost = 0, m_supply = 0; };
    std::uint32_t m_targetGuid = 0; int m_gossipEntry = 0;
    std::vector<int> m_questOffers, m_questCompletes, m_gossipOptions;
    std::vector<VendorSlot> m_vendorItems;
    static void readIntList(StlBuffer& d, std::vector<int>& out) {
        std::uint32_t n = 0; d >> n; for (std::uint32_t i = 0; i < n; ++i) { int v; d >> v; out.push_back(v); }
    }
    void unpack(StlBuffer& d) {
        d >> m_targetGuid >> m_gossipEntry;
        readIntList(d, m_questOffers);
        readIntList(d, m_questCompletes);
        readIntList(d, m_gossipOptions);
        // Vendor slot wire layout (locked from live op110 capture, Clive vendor: 33B / 3 slots = 11B each):
        //   u32 itemId, u8 flag, u8 supply, i32 cost, u8 pad.  i32 cost VERIFIED vs item_template buy
        //   (Tome 939=45, Scroll 1095=165, Ration 1110=15); supply 0xff=unlimited / scroll=5.
        //   (Reconstructed 12B over-read → underflow → menu never opened; then cost read 1B too early
        //   → inflated buy price 11775/42245/4095.)
        std::uint32_t n = 0; d >> n;
        for (std::uint32_t i = 0; i < n; ++i) {
            VendorSlot s;
            std::uint32_t itemId = 0; std::uint8_t flag = 0, supply = 0, pad = 0; std::int32_t cost = 0;
            d >> itemId >> flag >> supply >> cost >> pad;
            s.m_itemId = static_cast<int>(itemId);
            s.m_cost = cost;
            s.m_supply = supply;
            m_vendorItems.push_back(s);
        }
    }
};
struct GP_Server_UpdateVendorStock : GamePacket {
    ItemDef m_itemId; int m_amount = 0;
    // GROUND TRUTH (live op127 capture, 7B): itemId:u16, affix:u16, affixScore:u8, u8, stock:u8.
    //   (Verified: itemId 1095 = Scroll of Courage, stock decremented 4->3->2 as it was bought.)
    void unpack(StlBuffer& d) {
        std::uint16_t itemId = 0, affix = 0; std::uint8_t affixScore = 0, b = 0, stock = 0;
        d >> itemId >> affix >> affixScore >> b >> stock;
        m_itemId.m_itemId = itemId; m_itemId.m_affixId = affix; m_itemId.m_affixScore = affixScore;
        m_amount = stock;
    }
};
struct GP_Server_RepairCost : GamePacket {
    bool m_finished = false; int m_amount = 0;
    // Wire (live op128 capture, 5B): i32 amount, u8 finished. Reconstructed read them reversed
    // → m_finished got the amount's low byte (=0x10 truthy) → handler took the "done" branch → no cost popup.
    void unpack(StlBuffer& d) { d >> m_amount >> m_finished; }
};

// ---- Quests ------------------------------------------------------------------
struct GP_Server_QuestList : GamePacket {
    struct QuestEntry {
        int id = 0; bool done = false;
        std::vector<std::pair<int, int>> tallyItems, tallyNpcs, tallyGameObjects, tallySpells; // (entry, count)
    };
    std::vector<QuestEntry> m_quests;
    // GROUND TRUTH (live op111 capture, probe-confirmed): count:u8, quests×{ id:u32, done:u8,
    //   4 tally lists each [count:u8, pairs×{entry:u32, count:u8}] }. (Verified: questId 103, list
    //   entries 1020/5.) The decompile-era guess oversized counts to u32 — this is the real layout.
    static void readPairList(StlBuffer& d, std::vector<std::pair<int, int>>& out) {
        std::uint8_t n = 0; d >> n;
        for (std::uint8_t i = 0; i < n; ++i) { std::uint32_t e = 0; std::uint8_t c = 0; d >> e >> c; out.emplace_back((int)e, (int)c); }
    }
    void unpack(StlBuffer& d) {
        std::uint8_t n = 0; d >> n;
        for (std::uint8_t i = 0; i < n; ++i) {
            QuestEntry q; std::uint32_t id = 0; std::uint8_t done = 0; d >> id >> done; q.id = (int)id; q.done = (done != 0);
            readPairList(d, q.tallyItems);
            readPairList(d, q.tallyNpcs);
            readPairList(d, q.tallyGameObjects);
            readPairList(d, q.tallySpells);
            m_quests.push_back(q);
        }
    }
};
struct GP_Server_QuestTally : GamePacket {
    int m_type = 0, m_questId = 0, m_entry = 0, m_tally = 0;
    // GROUND TRUTH (live op113 capture, 13B): questId:u32, entry:u32, tally:u32, type:u8.
    //   (Verified: questId 103, entry 5, tally 1, type 0.)
    void unpack(StlBuffer& d) { std::uint8_t type = 0; d >> m_questId >> m_entry >> m_tally >> type; m_type = type; }
};
struct GP_Server_AcceptedQuest : GamePacket {
    int m_questId = 0;
    void unpack(StlBuffer& d) { d >> m_questId; }
};
struct GP_Server_QuestComplete : GamePacket {
    int m_questId = 0; bool m_done = false;
    void unpack(StlBuffer& d) { d >> m_questId >> m_done; }
};
struct GP_Server_RewardedQuest : GamePacket {
    int m_questId = 0;
    void unpack(StlBuffer& d) { d >> m_questId; }
};
struct GP_Server_AbandonQuest : GamePacket {
    int m_questId = 0;
    void unpack(StlBuffer& d) { d >> m_questId; }
};

// ---- Notifications -----------------------------------------------------------
struct GP_Server_SpentGold : GamePacket {
    int m_count = 0;
    void unpack(StlBuffer& d) { d >> m_count; }
};
struct GP_Server_ExpNotify : GamePacket {
    int m_amount = 0, m_newLevel = 0;
    void unpack(StlBuffer& d) { d >> m_amount >> m_newLevel; }
};
struct GP_Server_AvailableWorldQuests : GamePacket {
    // vector<uint16_t> to match MapQuester::setAvailableQuests(const vector<uint16_t>&).
    std::vector<std::uint16_t> m_list;
    // Wire (op120): count:u8, ids×u16. (Was count:u32.)
    void unpack(StlBuffer& d) { std::uint8_t n = 0; d >> n; for (std::uint8_t i = 0; i < n; ++i) { std::uint16_t v; d >> v; m_list.push_back(v); } }
};
struct GP_Server_InspectReveal : GamePacket {
    // Handler unpacks but does not consume any fields. // TODO verify (likely guid + equipment)
    std::uint32_t m_guid = 0;
    void unpack(StlBuffer& d) { if (d.size() >= sizeof(std::uint32_t)) d >> m_guid; }
};
struct GP_Server_SocketResult : GamePacket { // also reused for EmpowerResult
    bool m_success = false;
    void unpack(StlBuffer& d) { d >> m_success; }
};
struct GP_Server_RespawnResponse : GamePacket {
    bool m_success = false;
    void unpack(StlBuffer& d) { d >> m_success; }
};
struct GP_Server_PkNotify : GamePacket {
    std::string m_playerName;
    void unpack(StlBuffer& d) { d >> m_playerName; }
};

// ---- Errors ------------------------------------------------------------------
struct GP_Server_ChatError : GamePacket {
    int m_code = 0;
    // Wire = u16 code (len=2). Reading int32 here threw StlBuffer underflow -> swallowed by processPacket.
    void unpack(StlBuffer& d) { std::uint16_t c = 0; d >> c; m_code = c; }
};
struct GP_Server_WorldError : GamePacket {
    int m_code = 0;
    // Wire = u16 code (len=2). Was int32 -> `d >> m_code` underflow-threw -> error()/toast never ran.
    void unpack(StlBuffer& d) { std::uint16_t c = 0; d >> c; m_code = c; }
};

// ---- Duels / aggro / level ---------------------------------------------------
struct GP_Server_OfferDuel : GamePacket {
    std::string m_challengerName;
    void unpack(StlBuffer& d) { d >> m_challengerName; }
};
struct GP_Server_AggroMob : GamePacket {
    std::uint32_t m_guid = 0;
    void unpack(StlBuffer& d) { d >> m_guid; }
};
struct GP_Server_LvlResponse : GamePacket {
    bool m_bool = false;
    void unpack(StlBuffer& d) { d >> m_bool; }
};

// ---- Waypoints / map markers -------------------------------------------------
struct GP_Server_QueryWaypointsResponse : GamePacket {
    // vector<int> to match MapQuester::startWaypointSelection(const vector<int32_t>&).
    std::vector<int> m_guids;
    // Wire (op130, GROUND TRUTH live capture, 14B): count:u16, count×u32 waypointIds (e.g. 3 -> 126,134,235).
    //   Reading count as u32 read 0x007e0003 = huge -> underflow -> threw -> empty waypoint list ->
    //   discovered waypoints all showed undiscovered (green !), and the corrupted selection state could
    //   cascade into a bad op46 on click.
    void unpack(StlBuffer& d) { std::uint16_t n = 0; d >> n; for (std::uint16_t i = 0; i < n; ++i) { int g; d >> g; m_guids.push_back(g); } }
};
struct GP_Server_DiscoverWaypointNotify : GamePacket {
    std::uint32_t m_guid = 0;
    void unpack(StlBuffer& d) { if (d.size() >= sizeof(std::uint32_t)) d >> m_guid; } // // TODO verify (may be empty)
};
struct GP_Server_MarkNpcsOnMap : GamePacket {
    // set<int> to match MapQuester::queueRevealNpcs(const set<int>&).
    std::set<int> m_npcs;
    // REVERTED to original u32 count (re-decoded count width was conf=M + unverified).
    void unpack(StlBuffer& d) { std::uint32_t n = 0; d >> n; for (std::uint32_t i = 0; i < n; ++i) { int v; d >> v; m_npcs.insert(v); } }
};

// ---- Party -------------------------------------------------------------------
struct GP_Server_PartyList : GamePacket {
    // vector<int> to match World::setParty(const vector<int>&, const int).
    // Wire CONFIRMED from steam capture (op133): leaderGuid:u32, count:u8, count×u32 memberGuid.
    //   e.g. 04000000 02 04000000 01000000 = leader=4, 2 members {4,1}. (Was: count:u32 first + leader
    //   last -> read leaderGuid(0x04) as a 4-member count and crashed/garbled the party list.)
    std::vector<int> m_members; std::uint32_t m_leaderGuid = 0;
    void unpack(StlBuffer& d) {
        d >> m_leaderGuid;
        std::uint8_t n = 0; d >> n;
        for (std::uint8_t i = 0; i < n; ++i) { int g; d >> g; m_members.push_back(g); }
    }
};
struct GP_Server_OfferParty : GamePacket {
    std::string m_inviterName;
    void unpack(StlBuffer& d) { d >> m_inviterName; }
};

// ---- Trade -------------------------------------------------------------------
struct GP_Server_TradeUpdate : GamePacket {
    // m_myItems / m_theirItems: list of (itemGuid, list of (item, stackSize)).
    struct ItemGroup { std::uint32_t itemGuid = 0; std::vector<std::pair<ItemDef, int>> items; };
    std::vector<ItemGroup> m_myItems, m_theirItems;
    int m_myMoney = 0, m_hisMoney = 0;
    bool m_myReady = false, m_theirReady = false;
    std::uint32_t m_partnerGuid = 0;
    // Wire base CONFIRMED from steam capture (op137): partnerGuid:u32, myMoney:u32, hisMoney:u32,
    //   myReady:u8, theirReady:u8, myItemCount:u16, [myItems], theirItemCount:u16, [theirItems].
    //   Empty trade = exactly 18B (4+4+4+1+1+2+2) — fits the captured packet. (Was: item-groups first +
    //   partnerGuid LAST -> read partnerGuid(0x04) as a 4-group count -> threw on the common 18B packet.)
    //   Item entry ~21B from a single 1-item sample (itemId=0x133f): itemId:u16, affix:u16, +17 opaque
    //   (full ItemDef+stack). // TODO verify item-entry layout with a multi-item trade capture.
    static void readItems(StlBuffer& d, std::vector<ItemGroup>& out) {
        std::uint16_t n = 0; d >> n;
        for (std::uint16_t i = 0; i < n && d.size() >= 4; ++i) {
            std::uint16_t itemId = 0, affix = 0; d >> itemId >> affix;
            ItemGroup g; g.itemGuid = itemId;
            ItemDef def; def.m_itemId = itemId; def.m_affixId = affix;
            g.items.push_back({ def, 1 });
            out.push_back(g);
            d.eraseFront(d.size() >= 17 ? 17 : d.size()); // consume rest of the ~21B entry
        }
    }
    void unpack(StlBuffer& d) {
        d >> m_partnerGuid >> m_myMoney >> m_hisMoney >> m_myReady >> m_theirReady;
        readItems(d, m_myItems);
        readItems(d, m_theirItems);
    }
};
struct GP_Server_TradeCanceled : GamePacket { // empty body
    void unpack(StlBuffer&) {}
};

// ---- Guild -------------------------------------------------------------------
struct GP_Server_GuildRoster : GamePacket {
    struct Member { std::uint32_t guid = 0; std::string name; std::uint8_t classId = 0, rank = 0, level = 0; bool online = false; };
    std::string m_guildName, m_motd;
    std::vector<Member> m_members;
    void unpack(StlBuffer& d) {
        d >> m_guildName >> m_motd;
        std::uint32_t n = 0; d >> n;
        for (std::uint32_t i = 0; i < n; ++i) { Member m; d >> m.guid >> m.name >> m.classId >> m.rank >> m.level >> m.online; m_members.push_back(m); }
    }
};
struct GP_Server_GuildInvite : GamePacket {
    int m_guildId = 0; std::string m_guildName;
    void unpack(StlBuffer& d) { d >> m_guildId >> m_guildName; }
};
struct GP_Server_GuildAddMember : GamePacket {
    std::string m_playerName;
    void unpack(StlBuffer& d) { d >> m_playerName; }
};
struct GP_Server_GuildRemoveMember : GamePacket {
    std::string m_playerName;
    void unpack(StlBuffer& d) { d >> m_playerName; }
};
struct GP_Server_GuildOnlineStatus : GamePacket {
    std::string m_playerName; bool m_online = false;
    void unpack(StlBuffer& d) { d >> m_playerName >> m_online; }
};
struct GP_Server_GuildNotifyRoleChange : GamePacket {
    std::string m_playerName; int m_role = 0;
    void unpack(StlBuffer& d) { d >> m_playerName >> m_role; }
};

// ---- Channels / respec -------------------------------------------------------
struct GP_Server_ChannelInfo : GamePacket {
    int m_myChannel = 0, m_channelSize = 0;
    std::vector<int> m_channels;
    void unpack(StlBuffer& d) {
        d >> m_myChannel >> m_channelSize;
        std::uint32_t n = 0; d >> n;
        for (std::uint32_t i = 0; i < n; ++i) { int v; d >> v; m_channels.push_back(v); }
    }
};
struct GP_Server_ChannelChangeConfirm : GamePacket {
    bool m_success = false;
    void unpack(StlBuffer& d) { d >> m_success; }
};
struct GP_Server_PromptRespec : GamePacket {
    int m_cost = 0;
    void unpack(StlBuffer& d) { d >> m_cost; }
};

// ---- Arena -------------------------------------------------------------------
struct GP_Server_ArenaQueued : GamePacket {
    bool m_joined = false;
    void unpack(StlBuffer& d) { d >> m_joined; }
};
struct GP_Server_ArenaReady : GamePacket { // empty body (handler only spawns popup)
    void unpack(StlBuffer&) {}
};
struct GP_Server_ArenaOutcome : GamePacket {
    bool m_won = false;
    void unpack(StlBuffer& d) { d >> m_won; }
};
struct GP_Server_ArenaStatus : GamePacket {
    bool m_hasBegun = false;
    void unpack(StlBuffer& d) { d >> m_hasBegun; }
};

// ---- Gameobjects -------------------------------------------------------------
struct GP_Server_UnlockGameObj : GamePacket {
    int m_objEntry = 0;
    void unpack(StlBuffer& d) { d >> m_objEntry; }
};
