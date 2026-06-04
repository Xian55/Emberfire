#pragma once
#include <cstdint>

// Util::maskHas — bitmask test. The client uses it on spell Attributes etc.
// Some TUs (e.g. Toolbar.cpp) include SpellDefines.h but not StringHelpers.h,
// so the helper is provided here too. A shared include-guard macro keeps the
// (identical) definition from clashing when both headers land in one TU.
#ifndef DMYST_HAVE_UTIL_MASKHAS
#define DMYST_HAVE_UTIL_MASKHAS
namespace Util {
    template <typename M, typename F>
    inline bool maskHas(M mask, F flag) {
        return (static_cast<unsigned long long>(mask) &
                static_cast<unsigned long long>(flag)) != 0ull;
    }
}
#endif

namespace SpellDefines
{
	static constexpr int NumEffectIdx = 3; // spell_template has effect1..effect3

	// DEFINITIVE from SERVER effect-factory switch (deps/ghidra/dump/decompiled.c @~0x576f00:
	// `switch(effectType){ case N: new Effect_X }`; case label == effect id). Cross-checked vs DB exemplars.
	// Gaps 9,12,15,16,20,21 -> Effect_NullEffect (unused in this content).
	enum Effects   // plain enum: client compares with int via ==; scoped access still valid in C++17
	{
		SchoolDamage   = 1,   // magic/spell damage (effectN_data1 = school)
		Teleport       = 2,
		ApplyAura      = 3,   // effectN_data1 = AuraType (see below)
		ManaDrain      = 4,
		HealthDrain    = 5,
		Heal           = 6,
		Resurrect      = 7,
		CreateItem     = 8,
		SummonNpc      = 10,  // data1 = npc entry
		RestoreMana    = 11,
		Dispel         = 13,
		WeaponDamage   = 14,  // physical weapon strike (data1 = school)
		ManaBurn       = 17,
		Threat         = 18,
		TriggerSpell   = 19,
		InterruptCast  = 22,
		SummonObject   = 23,
		ScriptEffect   = 24,
		KnockBack      = 25,
		ApplyAreaAura  = 26,
		HealPct        = 27,
		RestoreManaPct = 28,
		TeleportForward= 29,
		MeleeAtk       = 30,
		RangedAtk      = 31,
		LootEffect     = 32,
		Kill           = 33,
		Gossip         = 34,
		Inspect        = 35,
		ApplyGemSocket = 36,  // data1 = gem id 1..15
		Charge         = 37,
		Duel           = 38,
		SlideFrom      = 39,
		ApplyOrbEnchant= 40,  // data1 = orb tier
		LearnSpell     = 41,  // data1 = class spellbook index
		NearestWp      = 42,  // Effect_NearestWayp (Scroll of Returning)
		PullTo         = 43,
		DestroyGems    = 44,
		CombineItem    = 45,
		ExtractOrb     = 46
	};

	// Aura behavior types (effectN_data1 when Effects::ApplyAura / ApplyAreaAura).
	// DEFINITIVE from SERVER aura-factory switch (deps/ghidra/dump/decompiled.c @~0x49d000:
	// `switch(aura->type){ case N: new Aura_X }`; case == value). Cross-checked vs DB exemplars
	// (1=Blight, 2=potions, 3=Bellowing Roar, 4=Blessing of Defense, 6=Blessed Shield/absorb,
	// 13=Divine Protection/school-immune, 14=Demoralizing Shout, 15=Aegis of Valor).
	// FREEZE FIX: the prior `ModifyStat=0` ordering was WRONG (login buffs mapped to a CC/root aura ->
	// frozen + greyed abilities). These are the real ordinals. Gaps 7,8,9,16,17 -> Aura_NullAuraType.
	// Plain enum: client compares int m_auraEffect against AuraType::X via ==.
	enum AuraType
	{
		PeriodicDamage          = 1,
		PeriodicHeal            = 2,
		InflictMechanic         = 3,
		ModifyStat              = 4,
		ModifyStatPct           = 5,
		AbsorbDamage            = 6,
		PeriodicRestoreMana     = 10,
		ModifyMoveSpeedPct      = 11,
		MechanicImmunity        = 12,   // Aura_MechanicImmune
		SchoolImmunity          = 13,
		ModifyDamageDealtPct    = 14,
		ModifyDamageReceivedPct = 15,
		PeriodicMeleeDamage     = 18,
		Model                   = 19,
		PeriodicBurnMana        = 20,
		Proc                    = 21,
		ModifyHealingDealtPct   = 22,
		ModifyHealingRecvPct    = 23,
		PeriodicHealPct         = 24,
		PeriodicRestoreManaPct  = 25,
		// referenced by client/editors but NOT in the server dispatch switch (-> NullAuraType,
		// unused in this build). Parked on gap values; do not rely on these matching the wire.
		ModifyResistance        = 7,    // TODO unconfirmed (not in dispatch switch)
		ModifyMeleeSpeedPct     = 8,    // TODO unconfirmed
		ModifyRangedSpeedPct    = 9,    // TODO unconfirmed
		PeriodicTriggerSpell    = 16,   // TODO unconfirmed
		RepopOntopOfSelf        = 17    // TODO unconfirmed
	};

	// Combat hit outcome (op102 CombatMsg hitResult:u8). DEFINITIVE from the server hit-resolver
	// FUN_004a1640 (melee) / FUN_004a0fb0 (general) — GHIDRA_WORLD.md §3.4:
	//   0=Hit/Normal, 1=Miss, 2=Glancing, 4=Dodge, 5=Parry, 6=Block, 7=Crit, 8=Immune.
	// Was misaligned (Dodge=2/Block=4/Crit=9/Immune=6) so combat text+sounds showed the wrong result.
	// 3=Evade and 9/10=Resist/Absorb are NOT in the melee resolver -> best-effort gaps, TODO verify.
	enum HitResult   // plain enum: compared with int via ==
	{
		Normal = 0,
		Miss = 1,
		Glancing = 2,
		Evade = 3,    // TODO verify (gap; client uses Evade for the miss sound)
		Dodge = 4,
		Parry = 5,
		Block = 6,
		Crit = 7,
		Immune = 8,
		Resist = 9,   // TODO verify (past server-confirmed 0..8 range)
		Absorb = 10   // TODO verify
	};

	// spell_template.dispel DB values 0..4. Scoped + switch-case use.
	// Plain enum: editors call to_string(SpellDefines::DispelType::X) (needs enum->int conversion).
	enum DispelType
	{
		Physical = 0, // DB dispel in {0..4}; TODO verify name<->ordinal
		Magic = 1,    // TODO verify
		Curse = 2,    // TODO verify
		Disease = 3,  // TODO verify
		Poison = 4    // TODO verify
	};

	// spell_template.effectN_targetType DB distinct values: 1,2,3,13,14,15,17,19,20.
	// Plain enum: editors call to_string(SpellDefines::TargetType::X) (needs enum->int conversion).
	enum TargetType
	{
		Unit_Caster = 1,                  // TODO verify
		Unit_Any = 2,                     // TODO verify
		Unit_Hostile = 3,                 // TODO verify
		Unit_Friendly = 13,               // TODO verify
		Unit_AreaSrc_Hostile = 14,        // TODO verify
		Unit_AreaSrc_Friendly = 15,       // TODO verify
		Unit_AreaDst_Hostile = 17,        // TODO verify
		Unit_AreaDst_Friendly = 19,       // TODO verify
		Unit_AreaDst_Hostile_FromDst = 20,// TODO verify
		Unit_AreaDst_Friendly_FromDst = 21,// TODO verify
		Target_GameObject = 22,           // TODO verify
		Target_Item = 23                  // TODO verify
	};

	// AI target selection (npc_template spell target type). Ordinals TODO verify.
	// Plain enum: editors call to_string(SpellDefines::AiTargetType::X) (needs enum->int conversion).
	enum AiTargetType
	{
		Target_T_Victim = 0,            // TODO verify
		Target_T_RandomHated,
		Target_T_RandomFriendly,
		Target_T_MissingMostHpFriendly
	};

	// spell_template.attributes bitmask. 64-bit field (real game: Pick Lock = 0x4080400000).
	// MUST have a fixed 64-bit underlying type or MSVC keeps the unscoped enum 32-bit and TRUNCATES
	// the high-bit flags (1ull<<33/35/38) to 0 -> TargetsGround/TargetsItem/MouseoverTargeting all
	// become 0 -> socket cursor + Pick Lock + ground spells silently break.
	enum Attributes : unsigned long long
	{
		Passive = 1u << 0,                 // TODO verify
		Triggered = 1u << 1,
		NotInCombat = 1u << 2,
		NotInArena = 1u << 3,
		NotInDungeon = 1u << 4,
		TargetNotInCombat = 1u << 5,
		TargetPlayersOnly = 1u << 6,
		CantTargetSelf = 1u << 7,
		CanTargetDead = 1u << 8,
		// BINARY-CONFIRMED bit positions (client binary Toolbar::castSpell @decompiled.c:264782 tests
		// `(attr & 0x200000000)==0 && (attr & 0x4000000000)==0`; Inventory socket gate @135021 tests
		// high-dword bit 3 = attr bit 35). attributes is a 64-bit field -> these REQUIRE 1ull, since
		// 1u<<33/35/38 is UB/0 on a 32-bit shift. Earlier 32-bit guesses (TargetsGround=9, TargetsItem=10,
		// MouseoverTargeting=22) were WRONG: bit 22 is set on Sleep(245)/buff scrolls/Inspect/Offer Duel
		// (NOT a cursor flag) -> made Sleep wrongly ground-cast; bit 10 never matched gems -> socket
		// "Invalid Target". Data agrees: bit 33 = Ice Blast/Teleport/Illusion Gate (true ground-placed),
		// bit 35 = all 33 gem/orb spells (targetType 20), bit 38 = Pick Lock only.
		TargetsGround = 1ull << 33,        // press skill -> ground-location cursor (AoE/teleport placement)
		TargetsItem = 1ull << 35,          // right-click gem -> item cursor -> click socketed item (Inventory.cpp:118)
		MouseoverTargeting = 1ull << 38,   // press skill -> click-a-target cursor (Pick Lock); Toolbar.cpp:286
		AutoApproach = 1u << 12,
		AnimLockStart = 1u << 13,
		DontStopCastingSound = 1u << 14,
		PersistsThroughDeath = 1u << 15,
		OnePerCaster = 1u << 16,
		OnePerTarget = 1u << 17,
		NoThreat = 1u << 18,
		NoAggro = 1u << 19,
		NoGoLock = 1u << 20,
		NoSpellBonus = 1u << 21,
		NoHealBonus = 1u << 11,    // real bit unknown (was 1u<<22, reclaimed for MouseoverTargeting); parked on the vacated bit 11; unused in client gameplay
		CantCrit = 1u << 23,
		IgnoreArmor = 1u << 24,
		IgnoreResistances = 1u << 25,
		IgnoreInvulnerability = 1u << 26,
		IgnoreLOS = 1u << 27,
		ImpossibleMiss = 1u << 28,
		ImpossibleDodge = 1u << 29,
		ImpossibleParry = 1u << 30,
		ImpossibleBlock = 1u << 31,
		// flags below exceed 32 bits in source ordering -> stored separately; TODO verify packing
		IgnoreStun,
		IgnoreFear,
		IgnoreSleep,
		IgnoreConfused,
		IgnoreIncapacitated,
		IgnorePolymorph,
		SameStackForAllCasters // referenced by TableTemplateEditor.cpp; bit/ordinal TODO verify
	};

	// Crowd-control / movement mechanics applied by auras. Ordinals TODO verify.
	// Plain enum: editors call to_string(SpellDefines::Mechanics::X) (needs enum->int conversion).
	enum Mechanics
	{
		Stun = 0, // TODO verify
		Silence,
		Fear,
		Confused,
		Incapacitated,
		Pacify,
		Polymorph,
		Sleep,
		Root,
		Snare,
		Charging,
		Disrupt,
		Stealth
	};

	// Bitmask of mechanics that interrupt a cast (aura_interrupt_flags / cast_interrupt_flags).
	enum InterruptCauses
	{
		StartCasting = 0x01,     // TODO verify
		Movement = 0x02,         // TODO verify
		TakeDamage = 0x04,       // TODO verify
		TakeDamage_Direct = 0x08,// TODO verify
		DisruptMechanic = 0x10   // TODO verify
	};

	// Proc trigger condition flags. Bit positions TODO verify.
	enum ProcFlags
	{
		ProcFlag_HolderDealtDamage = 0x01, // TODO verify
		ProcFlag_HolderTookDamage = 0x02,  // TODO verify
		ProcFlag_HolderDodged = 0x04,      // TODO verify
		ProcFlag_HolderWasImmune = 0x08    // TODO verify
	};

	enum ProcType
	{
		ProcType_RemoveCharge = 0x01 // TODO verify
	};

	namespace Misc
	{
		static constexpr int MaxSpellLevel = 60; // TODO verify
	}

	// Hard-coded special spell ids (verified vs spell_template names).
	enum StaticSpells
	{
		MeleeSpell  = 81,   // Melee Swing
		RangedSpell = 82,   // Ranged Attack
		CritVisual  = 83,   // Crit Dummy
		LootVisual  = 84,   // Loot Sparkle
		LootUnit    = 85,   // Loot (corpses/units)
		LootGameObj = 127,  // GROUND TRUTH (live capture): "Interact" (spell 127) — opening a chest/crate/
		                    //   barrel casts 127, server replies OpenLootWindow. (Was 85=Loot, which is for
		                    //   corpses; GameObjects use Interact.) Locked ones use PickLock=273 first.
		Interact    = 127,  // = the GameObject interact spell (alias of LootGameObj).
		QuestReady  = 86,   // Quest Ready
		QuestDone   = 87,   // Quest Done
		NpcGossip   = 88,   // Npc Gossip
		OfferDuel   = 128,  // Offer Duel
		SleepRest   = 245,  // Sleep
		Lockpicking = 273   // Pick Lock
	};
	// Note: StaticSpells is a plain enum, so both SpellDefines::StaticSpells::LootUnit and the
	// unscoped SpellDefines::LootUnit / ::QuestDone / ::NpcGossip etc. forms the client uses resolve.
}

// ---------------------------------------------------------------------------
// Reconstructed helper namespace (originally a transitively-included header).
// ---------------------------------------------------------------------------
namespace SpellFunctions
{
	// True if a spell id is one of the hard-coded "static" engine spells
	// (melee/ranged/loot/gossip/etc.) that the client must not place on the
	// action bar as a normal ability. TODO match server
	inline bool isStaticSpell(int spellId)
	{
		switch (spellId)
		{
		case SpellDefines::MeleeSpell:
		case SpellDefines::RangedSpell:
		case SpellDefines::LootUnit:
		case SpellDefines::LootGameObj: // = Interact (127) — GameObject open/loot
		case SpellDefines::LootVisual:
		case SpellDefines::CritVisual:
		case SpellDefines::Lockpicking:
		case SpellDefines::OfferDuel:
		case SpellDefines::SleepRest:
		case SpellDefines::NpcGossip:
		case SpellDefines::QuestReady:
		case SpellDefines::QuestDone:
			return true;
		default:
			return false;
		}
	}
}
