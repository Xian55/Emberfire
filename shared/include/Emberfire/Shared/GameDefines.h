#pragma once
#include <cstdint>

// GameDefines.h exposes two namespaces the client references: ObjDefines:: (object state variables,
// gossip status) and GameDefines:: (zone types). ObjDefines::Variable is the big sparse int enum
// synced over the wire (Server_ObjectVariable). Many ids are LOCKED by live capture (PROTOCOL.md).
// Client uses Variable both scoped (ObjDefines::Variable::Health) and unscoped (ObjDefines::ModelId,
// ObjDefines::PvP, ObjDefines::DynLootable, ObjDefines::DynGossipStatus) and does arithmetic on it
// (StatsStart + Stat), so it is a plain enum.

namespace ObjDefines
{
	enum Variable
	{
		// --- core object/unit vars (confirmed live) ---
		Mana = 0x00,           // GROUND TRUTH (live capture): current mana = var 0x00 (decreases on cast:
		                       //   225->153 while casting; max = var 0x0f=234). Was 0x04, which the server
		                       //   NEVER updates → mana display was frozen at the stale initial max.
		Health = 0x01,         // current health (confirmed: var 0x01 fluctuates 530-545 in combat)
		Level = 0x02,          // (confirmed: Yii=1, Baday=5)
		MaxHealth = 0x03,      // (confirmed)
		// 0x05..0x09 confirmed-zero-for-players (combat/pvp/flag block); names TODO verify
		InCombat = 0x05,       // TODO verify
		PvP = 0x06,            // TODO verify
		FactionId = 0x07,      // TODO verify
		ZoneType = 0x08,       // TODO verify
		ToggleState = 0x09,    // TODO verify (GameObject open/closed state)
		MoveSpeedPct = 0x0a,   // (confirmed: 100 = normal)
		// 0x0b/0x0c are unidentified aura/combat-state vars (0x0b = 0/0x20/0x30 bitmask during a DoT
		// debuff; wrongly guessed as ModelId, which caused the in-combat model glitch). Left unhandled.
		// ModelId/ModelScale real ids recovered from decompile FUN_0054d150 (@309024): 0xb0 / 0x0d.
		AuraState0b = 0x0b,    // TODO identify (NOT a model)
		Unknown_0c = 0x0c,     // (was GoFlags guess — real GoFlags is 0xb5, ground truth)
		ModelScale = 0x0d,     // CONFIRMED: FUN_0054d150 case 0x0d (model height-scale = value/100)

		// --- stat block: contiguous, maps 1:1 to UnitDefines::Stat (index = var - StatsStart) ---
		StatsStart = 0x0e,     // LOCKED (equip experiment): Health stat lives at 0x0e
		StatsEnd = 0x4d,       // TODO verify upper bound (StatsStart + UnitDefines::NumStats - 1)

		// 0xa0/0xa1 were GUESSED as DynGossipStatus/DynLootable — the real ids are 0xb9/0xb8
		// (decompile FUN_0054d150 cases). Names moved to the 0xb0 block below.
		Unknown_a0 = 0xa0,     // TODO verify
		Unknown_a1 = 0xa1,     // TODO verify
		DynGreyTagged = 0xa2,  // TODO verify
		Progression = 0xa3,    // (confirmed: Baday=157)
		Experience = 0xa4,     // (confirmed: Baday=124)
		Unknown_a5 = 0xa5,     // (was DynInteractable guess — real id is 0xbb, ground truth)
		DynSparkle = 0xa6,     // TODO verify
		NumInvestedSpells = 0xa7, // (confirmed: Baday=5)
		IsAnimating = 0xa8,    // TODO verify
		IsSliding = 0xa9,      // TODO verify
		MailLoot = 0xaa,       // TODO verify
		MechanicMask = 0xab,   // TODO verify
		LockpickingLevel = 0xb6,  // GROUND TRUTH (op79 + game.db): required lockpick skill, var 0xb6 =
		                          //   gameobject_template.data3*2 (Aspen Chest data3=3 -> 6, "Lv4" data3=4 -> 8).
		                          //   Was 0xac, which the server never sets -> tooltip read "Lockpicking (0)".
		                          //   Same id as Elite (0xb6) but different object class (unit elite vs GO lock).
		CombatRating = 0xad,   // TODO verify

		// --- 0xb0.. block ---  (ModelId/DynLootable/DynGossipStatus CONFIRMED from decompile FUN_0054d150)
		ModelId = 0xb0,        // CONFIRMED: case 0xb0 -> npc_models lookup (runtime model / polymorph)
		ArenaRating = 0xb1,    // TODO verify
		InArenaQueue = 0xb2,   // TODO verify
		PkCount = 0xb3,        // TODO verify
		ReactionMask_In = 0xb4,// TODO verify (=1 on all GOs / =2 Portal; client uses for spell-icon darken)
		GoFlags = 0xb5,        // GROUND TRUTH: op79 var 0xb5 == gameobject_template.flags exactly
		                       //   (Aspen Chest=9, Crate/Barrel/Orchid=1, Waypoint=6, Portal=2). Bit0
		                       //   (ShowName) gates the overhead name in ClientGameObj::render.
		                       //   (was guessed 0x0c which read 0 -> no name.)
		Elite = 0xb6,          // TODO verify
		Boss = 0xb7,           // TODO verify
		DynLootable = 0xb8,    // CONFIRMED: case 0xb8 -> loot-sparkle aura (LootVisual=84)
		DynGossipStatus = 0xb9,// CONFIRMED: case 0xb9 -> gossip status (1=QuestAvailable, 2=QuestComplete)
		Moderator = 0xba,      // TODO verify
		DynInteractable = 0xbb,// GROUND TRUTH (chest capture): =1 ONLY on interactable containers
		                       //   (Aspen Chest/Crate/Elm Barrel entries 1/10/17); absent (=0) on
		                       //   orchid doodad + waypoint/portal. Drives ClientGameObj::interactable().
		// These client-referenced names were wrongly placed at 0xb0/0xb8/0xb9 (= ModelId/DynLootable/
		// DynGossipStatus). Parked at placeholder ids until their real ids are recovered.
		CombatRatingMax = 0xf0,// TODO verify (was wrongly 0xb0)
		GameMaster = 0xf1,     // TODO verify (was wrongly 0xb8)
		GmInvisible = 0xf2,    // TODO verify (was wrongly 0xb9)
		ReactionMask_Out = 0xf3, // real id unknown (was 0xb5 = actually GoFlags); parked, client uses for spell-icon darken
		Money = 0x9e,          // (confirmed: Baday=1430)
	};

	// Dynamic gossip-status values held by Variable::DynGossipStatus. Client compares against
	// ObjDefines::GossipNone (unscoped) and ObjDefines::GossipStatus::QuestAvailable/QuestComplete.
	enum GossipStatus
	{
		// CONFIRMED via decompile FUN_0054d150 case 0xb9: param_3==1 -> QuestReady, ==2 -> QuestDone.
		GossipNone = 0,
		QuestAvailable = 1,    // "!" marker (QuestReady)
		QuestComplete = 2,     // "?" marker (QuestDone)
		GossipAvailable = 3    // TODO verify (talk-only gossip)
	};
}

namespace GameDefines
{
	// Zone PvP type, carried in ObjDefines::Variable::ZoneType. Client switches over all four.
	enum ZoneTypes
	{
		Friendly = 0,  // TODO verify
		Neutral = 1,   // TODO verify
		Hostile = 2,   // TODO verify
		Contested = 3  // TODO verify
	};
}
