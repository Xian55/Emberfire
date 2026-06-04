#pragma once
#include <cstdint>

namespace PlayerDefines
{
	// Class id. Carried in Server_CharacterList.classId:u8 and item_template.required_class (DB 1..4).
	// Client uses both PlayerDefines::Classes::Mage (scoped, casts wire int) and PlayerDefines::Mage
	// (unscoped) -> plain enum. DB required_class values 1..4 imply 1-based ids.
	enum Classes
	{
		// VERIFIED via player_class_stats + live chars (Baday=cls1=Paladin, Xii/Yii=cls3=Ranger):
		Paladin = 1, // class_stats: HP75/Str15/Courage15 (tank)
		Mage    = 2, // class_stats: Mana70/Int20 (caster)
		Ranger  = 3, // class_stats: Agi15 balanced
		Bishop  = 4, // class_stats: Mana65/Will20 (healer)
		NumClasses
	};

	// Carried in Server_CharacterList.gender:u8 (capture: 0). Both scoped and unscoped forms used.
	enum Gender
	{
		Male = 0,   // confirmed (capture gender=0)
		Female = 1, // TODO verify
		NumGenders
	};

	// Soft, client-side chat/social error notifications (PlayerDefines::ChatError::X, used in switch).
	// Plain enum (not enum class): the client passes these to World::chatError(const int),
	// which needs the implicit enum->int conversion. Scoped access (ChatError::X) still works.
	enum ChatError
	{
		PlayerNotFound = 0,        // TODO verify all ordinals
		ChatIgnored,
		CantInviteSelf,
		PlayerGroupedAlready,
		PartyFull,
		PartyDeclined,
		TargetBusy,
		AlreadyInGuild,
		TargetAlreadyInGuild,
		NotInGuild,
		GuildFull,
		GuildNameTaken,
		GuildDisbanded,
		InsufficientGuildRank,
		CantLeaveAsLeader,
		CantAcceptQuest,
		InstanceFull,
		InstanceInCombat,
		TradeInvalid,
		ErrorTransferingMap,
		DuelDeclined,
		DuelBegin,
		DuelCount1,
		DuelCount2,
		DuelCount3
	};

	// World/spell action errors (red error text). PlayerDefines::WorldError::X, used in World::error switch.
	// DEFINITIVE ordinals: cross-referenced the World::error switch (client/src/World.cpp:2073,
	// name->string) with the binary case#->string in dump_client/decompiled.c @0x295973 (string-table 0x6413xx).
	// Cases 2..35, 42, 43 CONFIRMED. Was guessed sequential (OutOfRange=0) -> client-side world->error() calls
	// (e.g. Game.cpp:1167 InvalidTarget) showed the WRONG toast. Plain enum: passed to World::error(const int).
	enum WorldError
	{
		NotHighEnoughLevel     = 40,  // "You are not a high enough level"
		SpellRequirementNotMet = 41,  // "Requirement for that spell is not met"
		CantEquipItem          = 2,   // "Unable to Equip Item"
		NotWhileInCombat       = 3,   // "You're in Combat"
		SpellNotReady          = 4,   // "Spell isn't ready"  (the err4 cooldown spam)
		CantWhileMoving        = 5,
		LineOfSight            = 6,   // "Not in Line of Sight"
		CasterDead             = 7,   // "You are Dead"
		TargetDead             = 8,   // "Target is Dead"
		InvalidTarget          = 9,
		NotWhileTargetInCombat = 10,  // "Target's in Combat"
		TargetPlayersOnly      = 11,  // "Can only Target Players"
		OutOfRange             = 12,
		TooClose               = 13,  // "You are too close"
		NotEnoughMana          = 14,
		EquipItemRequired      = 15,  // "Missing required Equiped Item"
		EquipItemBroken        = 16,  // "Required Item is Broken"
		CastInProgress         = 17,  // "Cast already in progress"
		Stunned                = 18,
		Sleeping               = 19,
		Incapacitated          = 20,
		Confused               = 21,
		Feared                 = 22,
		Polymorphed            = 23,
		Pacified               = 24,
		Charging               = 25,
		Rooted                 = 26,
		Silenced               = 27,
		// (code 28 has no case in the binary switch -> default "Unknown Error")
		QuestNotDone           = 29,  // "Quest not complete"
		InventoryFull          = 30,
		MaximumQuestsTracked   = 31,
		RewardNotChosen        = 32,  // "Choose a reward"
		OutOfStock             = 33,
		NotEnoughGold          = 34,
		VendorNotWorth         = 35,  // "The vendor doesn't want that item"
		// ===== Codes 36-52 GROUND TRUTH from the client World::error switch (dump_client/decompiled.c
		// FUN_0053f530 @0x295973, string-table 0x6413xx). Each maps to the exact in-binary message;
		// replaces the earlier best-effort guesses (which fell to default "Unknown Error"). NotHighEnoughLevel
		// (40) + SpellRequirementNotMet (41) are declared at the top of the enum. =====
		BuybackEmpty           = 36,  // "Nothing to buy back"
		MissingItem            = 37,  // "Missing an item"
		PerceptionFail         = 38,  // "Perception skill too low"
		AlreadyLooted          = 39,  // "Already looted"
		EnchantLimit           = 42,  // "Maximum potential reached for an item of that quality"
		NoEmptySockets         = 43,  // "No empty sockets in that item"
		NotEnoughExp           = 44,  // "Not enough Experience" (also the op40 LevelUp-overspend code)
		MagicSchoolLock        = 45,  // "Unable to use that School of Magic"
		LockpickTooLow         = 46,  // "Lockpicking not high enough"
		WrongClass             = 47,  // "You are not the right Class"
		AlreadyKnowSpell       = 48,  // "You already know that ability"
		RequiresLockPicking    = 49,  // "It's locked, try using Pick Lock"
		ItemIsSoulBound        = 50,  // "That item is soulbound"
		CantInArena            = 51,  // "You can't do that in an Arena."
		CantInDungeon          = 52,  // "You can't do that in a Dungeon."
	};

	namespace Experience
	{
		static constexpr int MaxLevelDiffExp = 10; // TODO verify
	}

	namespace Inventory
	{
		// VERIFIED: Inventory.h/Bank.h name the last slot "Slot49" and both attachIcon
		// ASSERT(Interface::Slot49 == 48), i.e. Slot49 = NumSlots-1 = 48 -> 49 slots each.
		static constexpr int NumSlots = 49;
		static constexpr int NumSlotsBank = 49;
	}

	namespace Trade
	{
		static constexpr int TradeDistance_Cells = 5; // TODO verify
	}
}

// ---------------------------------------------------------------------------
// PlayerFunctions  (reconstructed helper namespace)
//
// Class/stat helper functions. Reconstructed from client call sites
// (CharacterCreation.cpp, CharacterSelection.cpp, ClientUnit.cpp, ContentMgr.cpp,
// Equipment.cpp, GuildRoster.cpp, ItemIcon.cpp, ItemTemplateEditor.cpp, World.cpp,
// Abilities.cpp). Every consumer transitively includes this header (via
// ClientPlayer.h / GuildRoster.h / ContentMgr.h / CharacterCreation.h -> PlayerDefines.h),
// so defining the namespace here makes the helpers visible at all call sites
// without editing the client .cpp files.
//
// Signatures inferred from call sites:
//   className(Classes)                                   -> std::string
//   classColor(Classes)                                  -> uint32_t  (packed RGBA; wrapped in sf::Color(...))
//   canEquipItem(Classes, ItemDefines::WeaponType)       -> bool
//   canEquipItem(Classes, ItemDefines::EquipType)        -> bool
//   getRequiredStats(Classes, ArmorType, EquipType)      -> std::map<UnitDefines::Stat,int>
//   getStatModifier(Classes, UnitDefines::Stat, std::vector<UnitDefines::Stat>& out) -> void
//   getBarteringPcts(float* buyPct, float* sellPct, int barteringStat)  -> void
//   applyItemGoldValueScales(int* value, int level, int flags)          -> void
//   applyBarterting(int* buyPrice, int* sellPrice, int barteringStat)   -> void
//   computeStatUpgradeCost(UnitDefines::Stat, int totalInvested)        -> int  (compared < 0)
//   getStatUpgradeCap(UnitDefines::Stat)                                -> int
//   computeSpellUpgradeCost(int totalInvested)                          -> int
// ---------------------------------------------------------------------------
#include <string>
#include <map>
#include <vector>
#include <cmath>           // std::pow (computeStatUpgradeCost pow arms, FUN_005533f0)

#include "UnitDefines.h"   // UnitDefines::Stat
#include "ItemDefines.h"   // ItemDefines::WeaponType / EquipType / ArmorType

namespace PlayerFunctions
{
	inline std::string className(PlayerDefines::Classes classId)
	{
		switch (classId)
		{
		case PlayerDefines::Classes::Mage:    return "Mage";
		case PlayerDefines::Classes::Ranger:  return "Ranger";
		case PlayerDefines::Classes::Bishop:  return "Bishop";
		case PlayerDefines::Classes::Paladin: return "Paladin";
		default: break;
		}
		return "Unknown";
	}

	// Packed 0xRRGGBBAA value consumed by sf::Color(sf::Uint32). TODO match server (exact class colors).
	inline uint32_t classColor(PlayerDefines::Classes classId)
	{
		switch (classId)
		{
		case PlayerDefines::Classes::Mage:    return 0x3FC7EBFFu; // light blue
		case PlayerDefines::Classes::Ranger:  return 0xABD473FFu; // green
		case PlayerDefines::Classes::Bishop:  return 0xFFFFFFFFu; // white
		case PlayerDefines::Classes::Paladin: return 0xF58CBAFFu; // pink
		default: break;
		}
		return 0xFFFFFFFFu; // white
	}

	// Class weapon proficiency — gates which weapon-skill rows are GREEN/spendable (Equipment::canUseSkill).
	// GROUND TRUTH: client canUseSkill/canEquipItem = inlined FUN_0044f0d0 (@0x0044f0d0): wireId-0x1c selects
	// the weapon-skill row 0..7, class read from *(*(this+0xd0)+0x1ac) (1-based). Confirmed verbatim by the
	// CharacterCreation.cpp class tooltips: Paladin "Shields and Melee weapons", Mage "Staves and Wands",
	// Ranger "Bows and Daggers", Bishop "Shields, Staves and Maces".
	inline bool canEquipItem(PlayerDefines::Classes classId, ItemDefines::WeaponType weaponType)
	{
		using W = ItemDefines::WeaponType;
		if (weaponType == W::NoWeapon)
			return true; // not a weapon (armor / accessory) -> no weapon-type restriction
		switch (classId)
		{
			case PlayerDefines::Classes::Paladin:
				return weaponType == W::Mace || weaponType == W::Axe || weaponType == W::Sword;
			case PlayerDefines::Classes::Mage:
				return weaponType == W::Staff || weaponType == W::Wand;
			case PlayerDefines::Classes::Ranger:
				return weaponType == W::Bow || weaponType == W::Dagger;
			case PlayerDefines::Classes::Bishop:
				return weaponType == W::Staff || weaponType == W::Mace;
			default: break;
		}
		return false;
	}

	// Equip-slot proficiency. Only Shield is class-gated by the skill panel.
	// GROUND TRUTH: FUN_0044f0d0 case 7 (ShieldSkill) gates on class != Mage && class != Ranger, i.e.
	// Paladin + Bishop only. Confirmed by CharacterCreation tooltips (Paladin & Bishop "Can use Shields").
	// Other slots assume usable (server still validates).
	inline bool canEquipItem(PlayerDefines::Classes classId, ItemDefines::EquipType equipType)
	{
		if (equipType == ItemDefines::EquipType::Shield)
			return classId == PlayerDefines::Classes::Paladin
			    || classId == PlayerDefines::Classes::Bishop;
		return true;
	}

	// Per-class base-stat requirements for an item's armor material.
	// VERIFIED from server FUN_004b9660 (+ .rdata table 0x593110-0x593380): requirement = f(class, armorType);
	// equip slot is NOT used, and armor_type 10 (Diamond) has no requirement. Table stat ids are SPEND-IDs
	// (4=Strength,5=Agility,6=Willpower,7=Intelligence), converted here to UnitDefines::Stat for display.
	inline std::map<UnitDefines::Stat, int> getRequiredStats(PlayerDefines::Classes classId,
		ItemDefines::ArmorType armorType,
		ItemDefines::EquipType equipType)
	{
		(void)equipType;
		std::map<UnitDefines::Stat, int> req;
		const int a = static_cast<int>(armorType);
		if (a == ItemDefines::ArmorType::Diamond) // armor_type 10: server early-outs to no requirement
			return req;

		auto statOf = [](int spendId) -> UnitDefines::Stat {
			switch (spendId) {
				case 4:  return UnitDefines::Stat::Strength;
				case 5:  return UnitDefines::Stat::Agility;
				case 6:  return UnitDefines::Stat::Willpower;
				case 7:  return UnitDefines::Stat::Intelligence;
				default: return UnitDefines::Stat::Strength;
			}
		};
		auto put = [&](int spendId, int val) { if (val > 0) req[statOf(spendId)] = val; };

		switch (classId) {
		case PlayerDefines::Paladin: // class 1
			switch (a) {
			case 1:  put(7,1);   put(6,1);   break;
			case 2:  put(4,15);  put(5,10);  break;
			case 3:  put(4,18);  put(5,12);  break;
			case 4:  put(4,20);  put(5,14);  break;
			case 5:  put(4,25);  put(5,16);  break;
			case 6:  put(4,30);  put(5,18);  break;
			case 7:  put(4,35);  put(5,20);  break;
			case 8:  put(4,40);  put(5,35);  break;
			case 9:  put(4,60);  put(5,45);  break;
			case 11: put(4,100); put(5,75);  break;
			case 12: put(7,30);  put(6,30);  break;
			case 13: put(7,70);  put(6,70);  break;
			case 14: put(7,110); put(6,110); break;
			case 15: put(7,150); put(6,150); break;
			}
			break;
		case PlayerDefines::Mage: // class 2 (Intelligence caster)
			switch (a) {
			case 1:  put(7,1);   put(6,1);   break;
			case 2:  put(4,20);  break;
			case 3:  put(4,40);  break;
			case 4:  put(4,75);  break;
			case 5:  put(4,100); break;
			case 6:  put(4,125); break;
			case 7:  put(4,150); break;
			case 8:  put(4,175); break;
			case 9:  put(4,200); break;
			case 11: put(4,300); break;
			case 12: put(7,20);  put(6,16); break;
			case 13: put(7,30);  put(6,20); break;
			case 14: put(7,40);  put(6,35); break;
			case 15: put(7,70);  put(6,40); break;
			}
			break;
		case PlayerDefines::Ranger: // class 3
			switch (a) {
			case 1:  put(7,1);   put(6,1);   break;
			case 2:  put(5,16);  put(4,10);  break;
			case 3:  put(5,25);  put(4,12);  break;
			case 4:  put(5,30);  put(4,15);  break;
			case 5:  put(5,35);  put(4,20);  break;
			case 6:  put(5,40);  put(4,30);  break;
			case 7:  put(5,50);  put(4,40);  break;
			case 8:  put(5,100); put(4,50);  break;
			case 9:  put(5,150); put(4,150); break;
			case 11: put(5,300); put(4,300); break;
			case 12: put(7,30);  put(6,30);  break;
			case 13: put(7,70);  put(6,70);  break;
			case 14: put(7,110); put(6,110); break;
			case 15: put(7,150); put(6,150); break;
			}
			break;
		case PlayerDefines::Bishop: // class 4 (Willpower caster)
			switch (a) {
			case 1:  put(7,1);   break;
			case 2:  put(4,20);  break;
			case 3:  put(4,40);  break;
			case 4:  put(4,75);  break;
			case 5:  put(4,100); break;
			case 6:  put(4,125); break;
			case 7:  put(4,150); break;
			case 8:  put(4,175); break;
			case 9:  put(4,200); break;
			case 11: put(4,300); break;
			case 12: put(6,20);  put(7,16); break;
			case 13: put(6,30);  put(7,20); break;
			case 14: put(6,40);  put(7,35); break;
			case 15: put(6,70);  put(7,40); break;
			}
			break;
		default: break;
		}
		return req;
	}

	// Fills 'out' with the stats that modify 'stat' for this class. TODO match server.
	inline void getStatModifier(PlayerDefines::Classes classId, UnitDefines::Stat stat,
		std::vector<UnitDefines::Stat>& out)
	{
		(void)classId;
		(void)stat;
		// TODO match server: populate with the stats that scale this stat.
	}

	// Bartering buy/sell offset percentages (0..1) for a given Bartering stat value.
	// Either pointer may be null. TODO match server (exact curve).
	inline void getBarteringPcts(float* buyPct, float* sellPct, int barteringStat)
	{
		const float pct = static_cast<float>(barteringStat) * 0.001f; // 0.1% per point
		if (buyPct)  *buyPct  = pct;
		if (sellPct) *sellPct = pct;
	}

	// Scales an item's gold value by player level / item flags, in place.
	// TODO match server (exact scaling and which flag enables scaling).
	inline void applyItemGoldValueScales(int* value, int level, int flags)
	{
		if (!value)
			return;
		if (flags & ItemDefines::ItemFlags::ItemFlag_GoldValueScales)
			*value = static_cast<int>(static_cast<long long>(*value) * (level > 0 ? level : 1) / 100);
	}

	// Applies bartering discount/bonus to a buy and/or sell price, in place.
	// Either pointer may be null. TODO match server (exact formula).
	inline void applyBarterting(int* buyPrice, int* sellPrice, int barteringStat)
	{
		float buyPct = 0.f, sellPct = 0.f;
		getBarteringPcts(&buyPct, &sellPct, barteringStat);
		if (buyPrice)  *buyPrice  = static_cast<int>(*buyPrice  * (1.f - buyPct));
		if (sellPrice) *sellPrice = static_cast<int>(*sellPrice * (1.f + sellPct));
	}

	// Cost (points) to upgrade 'stat' given the amount already invested.
	// GROUND TRUTH: client computeStatUpgradeCost = FUN_005533f0 (@0x005533f0); switches on the WIRE stat id
	// (= Stat index + 1, see GP_Client_LevelUp::spendId) and returns -1 (default) for non-investable stats.
	// isAllow_AddStat hides the +button when this is < 0. Recovered byte-exact by disassembling 0x005533f0
	// (headless Ghidra, deps/ghidra/scripts/DumpStatCost.java) — the xmm pow constants the decompiler dropped:
	//   Health/Mana          (wire 1,2):  pow(n, n/1500.0f), result floored to >= 1   ([0x648b40]=1500.0f)
	//   Strength..Courage    (wire 4-8):  pow(n, 1.5)  + 5                              ([0x6489a0]=1.5)
	//   Regeneration/Meditate(wire 9,10): pow(n, 1.4f) + 4                              ([0x648998]=(double)1.4f)
	// The base is (double)(float)n and the pow result is narrowed to float before truncation (CVTSD2SS + ftol),
	// so we mirror the float round-trips to stay bit-faithful to the client/server cost math. Integer arms
	// (n/2+1, n/3+1, n+1, 2n+1) and the weapon-skill 3x-Strength recursion (disasm: ECX=4) are verified exact.
	inline int computeStatUpgradeCost(UnitDefines::Stat stat, int totalInvested)
	{
		using S = UnitDefines::Stat;
		const int n = totalInvested;
		// Mirror FUN_005533f0: r = (float)pow((double)(float)n, e); ftol truncates toward zero (n >= 0 so
		// == floor); final clamp to INT_MAX is the 0x7fffffff arm.
		auto powCost = [](int v, double e) -> int {
			const double r = std::pow(static_cast<double>(static_cast<float>(v)), e);
			const float rf = static_cast<float>(r);
			return rf >= 2147483648.0f ? 0x7fffffff : static_cast<int>(rf);
		};
		switch (stat)
		{
			case S::Health: case S::Mana:                                    // wire 1,2: pow(n, n/1500), floored at 1
			{
				const int v = powCost(n, static_cast<double>(static_cast<float>(n) / 1500.0f));
				return v > 1 ? v : 1;
			}
			case S::Strength: case S::Agility: case S::Willpower:
			case S::Intelligence: case S::Courage:                           // wire 4-8: pow(n, 1.5) + 5
				return powCost(n, 1.5) + 5;
			case S::Regeneration: case S::Meditate:                          // wire 9,10: pow(n, 1.4f) + 4
				return powCost(n, static_cast<double>(1.4f)) + 4;
			case S::MeleeCritical: case S::SpellCritical:                    // wire 15,17
				return n / 2 + 1;
			case S::RangedCritical:                                          // wire 16
			case S::ResistFrost: case S::ResistFire:                         // wire 21,22
			case S::ResistShadow: case S::ResistHoly:                        // wire 23,24
				return n / 3 + 1;
			case S::DodgeRating: case S::BlockRating:                        // wire 18,19
				return n + 1;
			case S::Bartering: case S::Lockpicking:                          // wire 25,26
				return n * 2 + 1;
			case S::StaffSkill: case S::MaceSkill: case S::AxesSkill: case S::SwordSkill:
			case S::RangedSkill: case S::DaggerSkill: case S::WandSkill: case S::ShieldSkill:
				return computeStatUpgradeCost(S::Strength, n) * 3;          // wire 28-35, 3x Strength (class-gated)
			default:                                                        // NOT investable: ArmorValue, MeleeValue,
				return -1;                                                   //   MeleeSpeed, RangedValue, RangedSpeed,
				                                                            //   ParryRating, Fortitude, Perception.
		}
	}

	// Maximum value a stat can be invested to (strict <: isAllow_AddStat tests base+bonus+pending < cap).
	// GROUND TRUTH: inlined in client isAllow_AddStat = FUN_0044ab30 (@0x0044ab30). Investable set mirrors
	// computeStatUpgradeCost; non-investable stats return -1 (belt-and-braces behind the cost<0 hide-guard).
	inline int getStatUpgradeCap(UnitDefines::Stat stat)
	{
		using S = UnitDefines::Stat;
		switch (stat)
		{
			case S::Health: case S::Mana:
				return 9999;
			case S::Bartering: case S::Lockpicking:
				return 100;
			case S::Strength: case S::Agility: case S::Willpower:
			case S::Intelligence: case S::Courage:
			case S::Regeneration: case S::Meditate:
			case S::MeleeCritical: case S::RangedCritical: case S::SpellCritical:
			case S::DodgeRating: case S::BlockRating:
			case S::ResistFrost: case S::ResistFire: case S::ResistShadow: case S::ResistHoly:
			case S::StaffSkill: case S::MaceSkill: case S::AxesSkill: case S::SwordSkill:
			case S::RangedSkill: case S::DaggerSkill: case S::WandSkill: case S::ShieldSkill:
				return 999;
			default:
				return -1;
		}
	}

	// Cost (points) for the next spell/talent upgrade given total invested. TODO match server.
	inline int computeSpellUpgradeCost(int totalInvested)
	{
		return 1 + totalInvested;
	}
}
