#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "StlBuffer.h"
// Client UI headers (TradeWindow.h, VendorNpc.h, ...) include only this Shared
// header but reference ItemDefines::ItemDefinition in their signatures. Chain it
// in here so those declarations resolve without editing the client. (ItemDefines
// pulls in UnitDefines; neither includes this file, so no cycle.)
#include "ItemDefines.h"
// Opcode values: those marked CONFIRMED were decoded from live capture (build/recon/OPCODES.md).
// The rest keep their real NAMES (from Game.cpp dispatch) but PLACEHOLDER values (>=1000) — // TODO verify
// by triggering each action and watching the opcode. Names are correct; only the unverified ints are guesses.

enum Opcode : std::uint16_t {
    // =========================================================================
    // CONFIRMED real values. Source: shipped client binary decompile
    // (build/recon/OPCODES_CLIENT.md). Each packet serialize writes its opcode
    // via FUN_004157a0(<op>) first; client->server in 1..74, server->client 75..150.
    // =========================================================================
    Mutual_Ping              = 1,
    Client_Authenticate      = 2,
    Server_Validate          = 3,
    Client_CharCreate        = 4,
    Client_EnterWorld        = 5,
    Client_ClickedGossipOption = 6,
    // VERIFIED vs steam client (capture conn1 18-58): THIS is the quest-accept the gossip flow uses.
    // op7 {u32 questId, u32 giverGuid} -> server FUN_00432c40 -> AcceptedQuest(112). giverGuid may be 0
    // (server resolves giver from the open GossipMenu session) or the npc guid; both accepted.
    Client_GossipQuestAccept = 7,
    // op8 = a DIFFERENT accept path (FUN_0048a030, reads 1 int). The gossip flow never hits it -> server
    // ignored our op8 sends (the "still cannot accept quest" bug). Kept for completeness; not used for accept.
    Client_AcceptQuest       = 8,
    Client_EquipItem         = 9,
    Client_DestroyItem       = 10,  // VERIFIED: op10 → server FUN_004931c0 prints "The item was destroyed" (NOT sell!).
                                    //   Wire = u8 slot + 11B item. Do NOT send this for selling — it destroys.
    Client_UseItem           = 11,
    Client_SellItem          = 12,  // GROUND TRUTH (live capture, money-verified across 21 sells): u8 slot, u16 itemId.
    Client_MoveItem          = 13,
    Client_UnequipItem       = 14,
    Client_RequestMove       = 15,
    Client_RequestStop       = 16,
    Client_ChatMsg           = 17,
    Client_GuildMotd         = 18,
    Client_AbandonQuest      = 19,
    Client_YieldDuel         = 21,
    Client_GuildInviteMember = 22,
    Client_GuildCreate       = 23,
    Client_GuildInviteResponse = 24,
    Client_GuildPromoteMember = 25,
    Client_GuildDemoteMember = 26,
    Client_CastSpell         = 27,
    Client_GuildRosterRequest = 28,
    Client_SetSelected       = 29,
    // op30 is LootItem (VERIFIED, client serialize FUN_004a0820). CancelBuff's real opcode is
    // unresolved — it was a wrong guess at 30; parked as a placeholder so it can't send the loot opcode.
    Client_CancelBuff        = 2005,
    Client_SetIgnorePlayer   = 31,
    Client_ReqTheoreticalSpell = 32,
    Client_CompleteQuest     = 33,
    Client_RecoverMailLoot   = 34,
    Client_BuyVendorItem     = 35,
    Client_Buyback           = 36,
    Client_Repair            = 37,
    Client_RequestRespawn    = 38,  // GROUND TRUTH (live capture): empty packet; server replies UnitTeleport+stat-restore. (was 43)
    Client_DuelResponse      = 39,
    Client_LevelUp           = 40,
    Client_SpellRankInvest   = 41,  // SpendExp: {u16 spellId, u16 targetRank}. Server FUN_004929a0. Sent per spell
                                    // BEFORE op40 LevelUp commits spell investments (original-client flow, capture conn1).
    Client_ReqAbilityList    = 42,
    Client_SortInventory     = 44,
    Client_QueryWaypoints    = 45,
    Client_ActivateWaypoint  = 46,
    Client_PartyInviteMember = 47,
    Client_PartyInviteResponse = 48,
    Client_PartyChanges      = 49,
    Client_ResetDungeons     = 58,  // GROUND TRUTH (live capture): empty packet.
    Client_SplitItemStack    = 66,  // GROUND TRUTH (live capture, Alt+click stack): u8 slot.
    Client_GuildQuit         = 50,
    Client_GuildDisband      = 51,
    Client_OpenTradeWith     = 52,
    Client_TradeAddItem      = 53,
    Client_TradeRemoveItem   = 54,
    Client_TradeCancel       = 55,
    Client_TradeConfirm      = 56,
    Client_TradeSetGold      = 57,
    Client_UnBankItem        = 59,
    Client_SortBank          = 60,
    Client_MoveBankToBank    = 61,
    Client_MoveInventoryToBank = 62,
    Client_ChangeChannels    = 63,
    Client_Respec            = 64,
    Client_DeleteCharacter   = 65,
    Client_LootItem          = 30,  // VERIFIED: client serialize FUN_004a0820 writes 0x1e (op66 is the Alt-click stack-loot variant)
    Client_UpdateArenaStatus = 67,
    Client_RollDice          = 68,
    Client_TogglePvP         = 69,
    Client_MOD_MutePlayer    = 70,
    Client_MOD_KickPlayer    = 71,
    Client_MOD_BanPlayer     = 72,
    Client_MOD_WarnPlayer    = 73,
    Client_ReportPlayer      = 74,

    // Server->client, decoded from the client receive dispatch FUN_00459b80 (direct switch, case==opcode).
    Server_CharacterList     = 75,
    Server_CharaCreateResult = 76,
    Server_NewWorld          = 77,   // == GP_Server_Player (entry spawn)
    Server_Npc               = 78,
    Server_GameObject        = 79,
    Server_SetController     = 80,
    Server_ObjectVariable    = 82,
    Server_InspectReveal     = 83,
    // op84 = S2C guid-only notify (proxy 'NotifyAdd?'); identity unconfirmed, no client struct. Listed for parity.
    Server_Inventory         = 85,
    Server_EquipItem         = 86,   // equip result (guid,slot,item) — sent live on equip
    Server_WorldError        = 87,
    Server_OfferDuel         = 88,
    Server_ChatError         = 89,
    Server_UnitSpline        = 90,
    Server_ChatMsg           = 91,
    Server_GuildAddMember    = 94,
    Server_GuildRoster       = 95,
    Server_SetSubname        = 96,
    Server_GuildInvite       = 97,
    Server_CastStart         = 98,
    Server_CastStop          = 99,
    Server_SpellGo           = 100,
    Server_UnitTeleport      = 101,
    Server_CombatMsg         = 102,
    Server_NotifyItemAdd     = 103,
    Server_OpenLootWindow    = 104,
    Server_UnitOrientation   = 105,
    Server_Cooldown          = 107,
    Server_UnitAuras         = 108,
    Server_Spellbook         = 109,
    Server_GossipMenu        = 110,
    Server_QuestList         = 111,
    Server_AcceptedQuest     = 112,
    Server_QuestTally        = 113,
    Server_QuestComplete     = 114,
    Server_RewardedQuest     = 115,
    Server_AbandonQuest      = 117,
    Server_ExpNotify         = 119,
    Server_AvailableWorldQuests = 120,
    Server_RespawnResponse   = 1007,  // UNCONFIRMED: op121 is SocketResult (server gem-slot handler sends it),
                                      // NOT respawn. Real respawn-response opcode unknown; respawn appears to
                                      // work via teleport/ObjVar packets. Parked in placeholder range (never sent).
    Server_Spellbook_Update  = 122,
    Server_LvlResponse       = 124,  // level-up / stat-spend result (built in the LevelUp handler)
    Server_AggroMob          = 125,
    Server_LearnedSpell      = 126,
    Server_UpdateVendorStock = 127,
    Server_RepairCost        = 128,
    Server_QueryWaypointsResponse = 130,
    Server_DiscoverWaypointNotify = 131,
    Server_OfferParty        = 132,
    Server_PartyList         = 133,
    Server_OnObjectWasLooted = 134,
    Server_MarkNpcsOnMap     = 135,
    Server_TradeUpdate       = 137,
    Server_TradeCanceled     = 138,
    Server_GuildNotifyRoleChange = 139,
    Server_GuildOnlineStatus = 140,
    Server_Bank              = 141,
    Server_OpenBank          = 142,
    Server_ChannelInfo       = 143,
    Server_PromptRespec      = 144,
    Server_ChannelChangeConfirm = 145,
    Server_ArenaReady        = 146,
    Server_SocketResult      = 121,  // wire-confirmed: server fills first empty gem slot then sends {ok:u8}
                                     // on op121 (decompiled.c socket handlers @41142/42132). 147 is an arena op.
    Server_ArenaOutcome      = 148,
    Server_ArenaStatus       = 149,
    Server_PkNotify          = 150,
    Server_QueuePosition     = 151,

    // =========================================================================
    // NOT YET CONFIRMED — names correct, values are placeholders. These packets
    // are decoded + assigned real opcodes per-system (Phases 2-11). Until then
    // sending a Client_* below disconnects; receiving a Server_* below is ignored.
    // Kept at 1000+/2000+ so they never collide with the real range (1..150).
    // =========================================================================
    // Still-placeholder server->client (no clean dispatch case found in the decode):
    Server_Player = 1000, Server_DestroyObject, Server_GuildRemoveMember, Server_SpentGold,
    Server_EmpowerResult, Server_UnlockGameObj, Server_ArenaQueued,

    // Still unresolved (NO confirmed client wire opcode): CancelCast + CancelBuff (likely server-side
    // Effect_InterruptCast / aura drop), SellItem (op10 turned out to be DestroyItem, not Sell — the
    // real sell opcode is NOT yet found), GuildKickMember. These MUST NOT be sent (out-of-range → server
    // drops the connection); guard their send sites.
    Client_CharacterList = 2000, Client_CancelCast, Client_GuildKickMember,
};

class GamePacket {
public:
    virtual ~GamePacket() = default;
    // Real opcodes live in 1..150; the 1000+/2000+ placeholders must never appear on the wire.
    static bool validOpcode(std::uint16_t op) { return op >= 1 && op <= 200; }
};

// ---- Decoded server packets (CONFIRMED layouts) ----
struct GP_Server_Validate : GamePacket {
    std::uint32_t m_result = 0, m_serverTime = 0, m_extra = 0;
    void unpack(StlBuffer& d) { d >> m_result >> m_serverTime >> m_extra; }
};

struct GP_Server_CharacterList : GamePacket {
    struct Entry { std::string name; std::uint8_t classId=0, level=0; std::uint32_t guid=0; std::uint8_t portrait=0, gender=0; };
    std::vector<Entry> m_characters;
    void unpack(StlBuffer& d) {
        std::uint32_t n=0; d >> n;
        for (std::uint32_t i=0;i<n;i++){ Entry e; d>>e.name>>e.classId>>e.level>>e.guid>>e.portrait>>e.gender; m_characters.push_back(e); }
    }
};

struct GP_Server_SetController : GamePacket {
    std::uint32_t m_guid = 0;
    void unpack(StlBuffer& d) { d >> m_guid; }
};

struct GP_Server_ObjectVariable : GamePacket {
    std::uint32_t m_guid = 0; std::uint8_t m_variableId = 0; std::int32_t m_value = 0;
    void unpack(StlBuffer& d) { d >> m_guid >> m_variableId >> m_value; }
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Server_ObjectVariable) << m_guid << m_variableId << m_value; m_buf=std::move(b); return m_buf; }
    StlBuffer m_buf;
};

struct GP_Server_ChatMsg : GamePacket {
    // Member names match the client (Game.cpp): m_fromGuid / m_fromName / m_itemId.
    std::uint32_t m_fromGuid = 0; std::uint8_t m_channelId = 0;
    std::string m_text, m_fromName;
    ItemDefines::ItemDefinition m_itemId; // optional linked item (m_itemId.m_itemId==0 means none)
    void unpack(StlBuffer& d) { d >> m_fromGuid >> m_channelId >> m_text >> m_fromName; /* TODO verify trailing item link */ }
};

// Object packets (Npc/NewWorld) carry: guid/mapId, x, y, [9-byte marker], [varId:u8,val:i32]* , orient:float, trailer.
// See build/recon/PROTOCOL.md. NewWorld additionally has name/gender/portrait/class/equipment after the var block.
// TODO: GP_Server_Npc / GP_Server_NewWorld / GP_Server_Player full structs (variable map + equipment list).

// Pull in the remaining ~134 GP_* packet classes so anything including GamePacketServer.h sees them all.
#include "GamePackets_Stubs.h"
