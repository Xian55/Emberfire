#pragma once
#include <cstdint>
#include <string>

namespace NpcDefines
{
	// npc_template.type DB values 0..4. Client uses NpcDefines::Type::Beast etc (scoped).
	// Plain enum: editors call to_string(NpcDefines::Type::X) (needs enum->int conversion).
	enum Type
	{
		// Ordinals derived from npc_template.type vs known NPCs: 1=Antling/spiders/howlers,
		// 2=cockatrice/wyrm/Tchazzar, 3=Lost Soul/The Damned, 4=Battle Mage/warriors/Vincent.
		Unknown   = 0,   // Huldria/Mastemia (special) — shows blank
		Beast     = 1,
		Dragonkin = 2,
		Undead    = 3,
		Humanoid  = 4
	};

	// npc_template.ai_type DB values 0..2. Scoped use (NpcDefines::AiType::MeleeAI).
	enum AiType
	{
		MeleeAI = 0,  // DB ai_type in {0,1,2}; TODO verify
		CasterAI = 1, // TODO verify
		ArcherAI = 2  // TODO verify
	};

	// npc_template.movement_type DB values 0..2. Scoped use (NpcDefines::DefaultMovement::Idle).
	enum DefaultMovement
	{
		Idle = 0,   // DB movement_type in {0,1,2}; TODO verify
		Random = 1, // TODO verify
		Patrol = 2  // TODO verify
	};

	// npc_template.npc_flags bitmask. Low bits CONFIRMED (DB + 130 named NPCs on wire: 0x1=Gossip/Guard,
	// 0x3=questgivers, 0x5=vendors/Boltar, 0x7=all). SpendExpCredit=0x800 confirmed ("Spend Experience
	// Points" npc=2048). DB only sets bits 0x1/0x2/0x4/0x200/0x400/0x800; 0x08/0x10 never used in content.
	enum Flags
	{
		Gossip = 0x01,
		QuestGiver = 0x02,
		Vendor = 0x04,
		TalkCredit = 0x200,     // TODO verify (0x200/0x400 order between TalkCredit/LevelToCredit)
		LevelToCredit = 0x400,  // TODO verify
		SpendExpCredit = 0x800  // confirmed (was wrongly 0x20)
	};

	// npc_template.game_flags bitmask (combat/AI behavior). Bit positions TODO verify.
	enum GameFlags
	{
		NoAggro = 0x01,      // TODO verify
		AggroZone = 0x02,    // TODO verify
		NoMove = 0x04,       // TODO verify
		NoMelee = 0x08,      // TODO verify
		NoFight = 0x10,      // TODO verify
		NoAssist = 0x20,     // TODO verify
		NoCallAssist = 0x40, // TODO verify
		NoTaunt = 0x80       // TODO verify
	};
}

// ---------------------------------------------------------------------------
// Reconstructed helper namespace (originally a transitively-included header).
// ---------------------------------------------------------------------------
namespace NpcFunctions
{
	// Display name for an NPC creature type (shown in unit tooltips: "Level N <type>").
	inline std::string typeName(NpcDefines::Type type)
	{
		switch (type)
		{
		case NpcDefines::Type::Unknown:   return "";
		case NpcDefines::Type::Humanoid:  return "Humanoid";
		case NpcDefines::Type::Beast:     return "Beast";
		case NpcDefines::Type::Undead:    return "Undead";
		case NpcDefines::Type::Dragonkin: return "Dragonkin";
		default:                          return "";
		}
	}
}
