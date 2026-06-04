#pragma once
#include <cstdint>
#include <string>

// Reconstructed from client usage + live capture (PROTOCOL.md "Stat block -> UnitDefines::Stat").
// The client uses both scoped (UnitDefines::Stat::Health) and unscoped (UnitDefines::Belt, ::North)
// forms, and does integer arithmetic on these (e.g. Stat(var - StatsStart)), so they are plain
// enums inside the namespace rather than enum class.
namespace UnitDefines
{
	// Stat order is LOCKED: index i maps to ObjDefines::Variable id (0x0e + i) (StatsStart = 0x0e).
	// Confirmed live via the player NewWorld var table + equip experiment (affix stat_type T -> var 0x0e+T).
	// Indices 0..28 are DB/capture-confirmed; the 0x2b+ skill/derived block order follows the
	// Equipment.cpp stat panel layout (skill tab) and is marked TODO where not independently verified.
	enum Stat
	{
		// GROUND TRUTH (op77 wire dump 2026-06-03 conn22-v1190/conn6-v1189 + r1189 stat panel + equip
		// experiment + 0x1a=2000=MeleeCooldown / 0x27=Bartering anchors): var = StatsStart(0x0e) + enum index,
		// fully contiguous. Measured: 0x0e=WeaponValue(non-spendable melee item dmg, 0 for casters),
		// 0x0f=Mana, 0x10=Health, 0x11=ArmorValue, 0x12=Strength ... 0x18=Meditate, 0x19=MeleeValue,
		// 0x1a=MeleeCooldown. The op40 SPEND-id order is DIFFERENT (Health=1,Mana=2,ArmorValue=3,Strength=4,
		// ... -- Health/Mana swapped vs the var order). That swap + the wire ids live ONLY in
		// GP_Client_LevelUp::spendId, never here. (Earlier this enum had ArmorValue=0 at 0x0e, so getStat read
		// one var too low and the whole stat panel was shifted by one row; fixed.) RangedWeaponValue=phantom.
		WeaponValue = 0,    // var 0x0e  item melee weapon damage (0 for casters); NON-spendable
		Mana = 1,           // var 0x0f  = MaxMana; spend-id 2
		Health = 2,         // var 0x10  = MaxHealth (mirror of 0x03); spend-id 1
		ArmorValue = 3,     // var 0x11  real armor (NOT investable); spend-id 3
		Strength = 4,       // var 0x12  spend-id 4
		Agility = 5,        // var 0x13  spend-id 5
		Willpower = 6,      // var 0x14  spend-id 6
		Intelligence = 7,   // var 0x15  spend-id 7
		Courage = 8,        // var 0x16  spend-id 8
		Regeneration = 9,   // var 0x17  spend-id 9
		Meditate = 10,      // var 0x18  spend-id 10
		MeleeValue = 11,    // var 0x19
		MeleeSpeed = 12,    // var 0x1a  (item alias "MeleeCooldown")
		RangedValue = 13,   // var 0x1b
		RangedSpeed = 14,   // var 0x1c  (item alias "RangedCooldown")
		MeleeCritical = 15, // var 0x1d  (spend id 15, n/2+1)
		RangedCritical = 16,// var 0x1e  (spend id 16, n/3+1)
		SpellCritical = 17, // var 0x1f  (spend id 17, n/2+1)
		DodgeRating = 18,   // var 0x20  (spend id 18, n+1)
		BlockRating = 19,   // var 0x21  (spend id 19, n+1)
		ParryRating = 20,   // var 0x22  (hidden in panel, NOT investable)
		ResistFrost = 21,   // var 0x23  (spend id 21, n/3+1)
		ResistFire = 22,    // var 0x24
		ResistShadow = 23,  // var 0x25
		ResistHoly = 24,    // var 0x26
		Bartering = 25,     // var 0x27  (spend id 25, 2n+1)   CONFIRMED conn37 var0x27=18
		Lockpicking = 26,   // var 0x28  (spend id 26, 2n+1)   CONFIRMED var0x28=19
		Fortitude = 27,     // var 0x29  (hidden label, NOT investable)  CONFIRMED var0x29=0
		StaffSkill = 28,    // var 0x2a  Staves   | spend id 28..35 weapon skills = 3x Strength,
		MaceSkill = 29,     // var 0x2b  Maces     |  client-gated by class.  CONFIRMED var0x2b=14
		AxesSkill = 30,     // var 0x2c  Axes      CONFIRMED var0x2c=15
		SwordSkill = 31,    // var 0x2d  Swords    CONFIRMED var0x2d=16
		RangedSkill = 32,   // var 0x2e  Ranged
		DaggerSkill = 33,   // var 0x2f  Daggers
		WandSkill = 34,     // var 0x30  Wands
		ShieldSkill = 35,   // var 0x31  Shields   CONFIRMED var0x31=17
		Perception = 36,    // var 0x32  (hidden, NOT investable)

		// Code-referenced names that are NOT real spendable stats (no tooltip file, not in spend block).
		// Parked high so var = StatsStart+idx falls outside [StatsStart,StatsEnd] and never aliases a stat.
		DodgeChanceBonus = 100,  // TODO real id
		ParryChanceBonus = 101,
		BlockChanceBonus = 102,
		NpcMeleeSkill    = 103,
		NpcRangedSkill   = 104,

		// Aliases used by the item system (ItemIcon/ItemGenerator) and the combat-tab labels. MeleeSpeed/
		// RangedSpeed = item "cooldown"; RangedWeaponValue has no own var so it maps to RangedValue.
		MeleeCooldown     = MeleeSpeed,   // melee attack speed  (var 0x1a)
		RangedWeaponValue = RangedValue,  // item ranged damage  (var 0x1b)
		RangedCooldown    = RangedSpeed,  // ranged attack speed (var 0x1c)

		NumStats = 37,      // count of real stats (Health..Perception incl. WeaponValue); placeholders/aliases excluded
		NullStat = -1       // sentinel: "no stat"
	};

	// 8-direction rotational order (used to index travel/anim arrays). Unscoped + scoped use.
	enum Direction
	{
		North = 0,
		NorthEast = 1,
		East = 2,
		SouthEast = 3,
		South = 4,
		SouthWest = 5,
		West = 6,
		NorthWest = 7,
		NumDirections
	};

	// Equipment slots. Client uses both UnitDefines::EquipSlot::Belt and unscoped UnitDefines::Belt,
	// and casts wire ints to EquipSlot, so plain enum. Values TODO verify (sequential from 0).
	// Equipment slot id (the op77 equipment slot byte + op9/op124 slot). VERIFIED by dumping op77
	// {slot,entry} vs DB equip_types: rings take TWO slots (8,9) so weapon/offhand/ranged shift up.
	//   1 Helm, 2 Necklace, 3 Chest, 4 Belt, 5 Legs, 6 Feet, 7 Hands, 8 Ring1, 9 Ring2,
	//   10 Weapon (main hand), 11 Offhand (shield), 12 Ranged. (NOT == equip_type, which lacks Ring2.)
	enum EquipSlot
	{
		NoEquipSlot = 0,
		Helm     = 1,
		Necklace = 2,
		Chest    = 3,
		Belt     = 4,
		Legs     = 5,
		Feet     = 6,
		Hands    = 7,
		Ring1    = 8,
		Ring2    = 9,
		Weapon1  = 10,
		Offhand  = 11,
		Ranged   = 12,
		NumEquipSlots
	};

	// Faction. npc_template.faction DB values seen: 1,2,3. Friendly/Hostile/Neutral confirmed present,
	// PlayerDefault/PvP extra. Exact ordinal->name binding TODO verify against server.
	enum Faction
	{
		// GROUND TRUTH: decompiled getFactionColor (FUN_0054eda0) switch + npc_template.faction +
		// op78 var 0xb1. case0=blue(PlayerDefault), 1=Green(Friendly), 2=Yellow(Neutral), 3=Red(Hostile),
		// 4=orange(PvP). Was Hostile/Neutral SWAPPED -> Howler(faction3) showed yellow instead of red;
		// Antling(faction2) is Neutral (user-confirmed).
		PlayerDefault = 0,
		Friendly = 1,
		Neutral = 2,
		Hostile = 3,
		PvP = 4
	};

	// Magic schools. DEFINITIVE from client school-name switch (dump_client @~0x239600) + effect1.data1
	// + cast_school exemplars. Was off-by-one (Physical=0). 0=none.
	enum MagicSchool
	{
		NullMagicSchool = 0,
		Physical = 1,
		Frost = 2,
		Fire = 3,
		Shadow = 4,
		Holy = 5,
		NumMagicSchools = 6
	};

	// Animation ids for unit sprites. VERIFIED from original client binary ModelScript::parse
	// (dump_client/decompiled.c ~L194037+): section-name -> id assignment, exact order/values.
	// DB spell_visual.unit_cast_animation/unit_go_animation store these raw ids (static_cast<AnimId>).
	// The earlier guessed order (Cast=4, CritDie=9, ...) made casts play the wrong section (id 9 = [block]
	// real, but [critdie] under the guess) at the wrong rate -> "cast animation sped up".
	enum AnimId
	{
		Stance  = 0,
		Spawn   = 1,
		Shoot   = 2,
		Run     = 3,
		Die     = 4,
		CritDie = 5,
		Cast    = 6,
		Swing   = 7,
		Hit     = 8,
		Block   = 9,
		CastAlt = 10,
		NumAnims
	};
}

// ---------------------------------------------------------------------------
// Reconstructed helper namespace (originally a transitively-included header).
// Pure display-string mappers over UnitDefines enums + no-side-effect helpers.
// ---------------------------------------------------------------------------
namespace UnitFunctions
{
	// Full display name for a stat. Used in tooltips, stat panels, item text.
	inline std::string statName(UnitDefines::Stat stat)
	{
		switch (stat)
		{
		// Names match scripts/text/stats/<name-without-spaces>.txt (the tooltip files = ground truth).
		case UnitDefines::Stat::Health:            return "Health";
		case UnitDefines::Stat::Mana:              return "Mana";
		case UnitDefines::Stat::ArmorValue:        return "Armor Value";
		case UnitDefines::Stat::Strength:          return "Strength";
		case UnitDefines::Stat::Agility:           return "Agility";
		case UnitDefines::Stat::Willpower:         return "Willpower";
		case UnitDefines::Stat::Intelligence:      return "Intelligence";
		case UnitDefines::Stat::Courage:           return "Courage";
		case UnitDefines::Stat::Regeneration:      return "Regeneration";
		case UnitDefines::Stat::Meditate:          return "Meditate";
		case UnitDefines::Stat::MeleeValue:        return "Melee Value";
		case UnitDefines::Stat::MeleeSpeed:        return "Melee Speed";
		case UnitDefines::Stat::RangedValue:       return "Ranged Value";
		case UnitDefines::Stat::RangedSpeed:       return "Ranged Speed";
		case UnitDefines::Stat::MeleeCritical:     return "Melee Critical";
		case UnitDefines::Stat::RangedCritical:    return "Ranged Critical";
		case UnitDefines::Stat::SpellCritical:     return "Spell Critical";
		case UnitDefines::Stat::DodgeRating:       return "Dodge Rating";
		case UnitDefines::Stat::BlockRating:       return "Block Rating";
		case UnitDefines::Stat::ParryRating:       return "Parry Rating";
		case UnitDefines::Stat::ResistFrost:       return "Resist Frost";
		case UnitDefines::Stat::ResistFire:        return "Resist Fire";
		case UnitDefines::Stat::ResistShadow:      return "Resist Shadow";
		case UnitDefines::Stat::ResistHoly:        return "Resist Holy";
		case UnitDefines::Stat::Bartering:         return "Bartering";
		case UnitDefines::Stat::Lockpicking:       return "Lockpicking";
		case UnitDefines::Stat::Fortitude:         return "Fortitude";
		case UnitDefines::Stat::StaffSkill:        return "Staves";
		case UnitDefines::Stat::MaceSkill:         return "Maces";
		case UnitDefines::Stat::AxesSkill:         return "Axes";
		case UnitDefines::Stat::SwordSkill:        return "Swords";
		case UnitDefines::Stat::RangedSkill:       return "Ranged";
		case UnitDefines::Stat::DaggerSkill:       return "Daggers";
		case UnitDefines::Stat::WandSkill:         return "Wands";
		case UnitDefines::Stat::ShieldSkill:       return "Shields";
		case UnitDefines::Stat::Perception:        return "Perception";
		default:                                   return "Unknown";
		}
	}

	// Short abbreviation for a stat (compact tooltips e.g. spell scaling "(STR)").
	inline std::string statAbbr(UnitDefines::Stat stat)
	{
		switch (stat)
		{
		case UnitDefines::Stat::Health:            return "HP";
		case UnitDefines::Stat::Mana:              return "MP";
		case UnitDefines::Stat::ArmorValue:        return "ARM";
		case UnitDefines::Stat::Strength:          return "STR";
		case UnitDefines::Stat::Agility:           return "AGI";
		case UnitDefines::Stat::Willpower:         return "WIL";
		case UnitDefines::Stat::Intelligence:      return "INT";
		case UnitDefines::Stat::Courage:           return "COU";
		case UnitDefines::Stat::Regeneration:      return "REG";
		case UnitDefines::Stat::Meditate:          return "MED";
		case UnitDefines::Stat::MeleeValue:        return "MEL";
		case UnitDefines::Stat::MeleeSpeed:        return "MSPD";
		case UnitDefines::Stat::RangedValue:       return "RNG";
		case UnitDefines::Stat::RangedSpeed:       return "RSPD";
		case UnitDefines::Stat::MeleeCritical:     return "MCRIT";
		case UnitDefines::Stat::RangedCritical:    return "RCRIT";
		case UnitDefines::Stat::SpellCritical:     return "SCRIT";
		case UnitDefines::Stat::DodgeRating:       return "DGE";
		case UnitDefines::Stat::BlockRating:       return "BLK";
		case UnitDefines::Stat::ResistFrost:       return "rFRO";
		case UnitDefines::Stat::ResistFire:        return "rFIR";
		case UnitDefines::Stat::ResistShadow:      return "rSHA";
		case UnitDefines::Stat::ResistHoly:        return "rHOL";
		case UnitDefines::Stat::Fortitude:         return "FOR";
		case UnitDefines::Stat::Bartering:         return "BAR";
		case UnitDefines::Stat::Lockpicking:       return "LCK";
		case UnitDefines::Stat::Perception:        return "PER";
		case UnitDefines::Stat::StaffSkill:        return "STF";
		case UnitDefines::Stat::MaceSkill:         return "MAC";
		case UnitDefines::Stat::AxesSkill:         return "AXE";
		case UnitDefines::Stat::SwordSkill:        return "SWD";
		case UnitDefines::Stat::RangedSkill:       return "RGD";
		case UnitDefines::Stat::DaggerSkill:       return "DGR";
		case UnitDefines::Stat::WandSkill:         return "WND";
		case UnitDefines::Stat::ShieldSkill:       return "SHD";
		case UnitDefines::Stat::ParryRating:       return "PRY";
		case UnitDefines::Stat::DodgeChanceBonus:  return "DGE%";
		case UnitDefines::Stat::ParryChanceBonus:  return "PRY%";
		case UnitDefines::Stat::BlockChanceBonus:  return "BLK%";
		case UnitDefines::Stat::NpcMeleeSkill:     return "nMEL";
		case UnitDefines::Stat::NpcRangedSkill:    return "nRGD";
		default:                                   return "?";
		}
	}

	// Equipment slot display name.
	inline std::string equipSlotName(UnitDefines::EquipSlot slot)
	{
		switch (slot)
		{
		case UnitDefines::EquipSlot::Weapon1:  return "Weapon";
		case UnitDefines::EquipSlot::Offhand:  return "Offhand";
		case UnitDefines::EquipSlot::Ranged:   return "Ranged";
		case UnitDefines::EquipSlot::Helm:     return "Helm";
		case UnitDefines::EquipSlot::Necklace: return "Necklace";
		case UnitDefines::EquipSlot::Chest:    return "Chest";
		case UnitDefines::EquipSlot::Belt:     return "Belt";
		case UnitDefines::EquipSlot::Legs:     return "Legs";
		case UnitDefines::EquipSlot::Feet:     return "Feet";
		case UnitDefines::EquipSlot::Hands:    return "Hands";
		case UnitDefines::EquipSlot::Ring1:    return "Ring";
		case UnitDefines::EquipSlot::Ring2:    return "Ring";
		default:                               return "Unknown";
		}
	}

	// Magic school display name.
	inline std::string magicSchoolName(UnitDefines::MagicSchool school)
	{
		switch (school)
		{
		case UnitDefines::MagicSchool::Physical: return "Physical";
		case UnitDefines::MagicSchool::Frost:    return "Frost";
		case UnitDefines::MagicSchool::Fire:     return "Fire";
		case UnitDefines::MagicSchool::Shadow:   return "Shadow";
		case UnitDefines::MagicSchool::Holy:     return "Holy";
		default:                                 return "Unknown";
		}
	}
}

// Editors (ItemGenerator.cpp) reference ItemDefines:: but only #include UnitDefines.h.
// ItemDefines depends on UnitDefines (above), so chain it here after the namespace is defined.
#include "ItemDefines.h"
