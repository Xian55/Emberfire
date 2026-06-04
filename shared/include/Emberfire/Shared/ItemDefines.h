#pragma once
#include <cstdint>
#include "UnitDefines.h" // ItemDefines may depend on UnitDefines::Stat (allowed)

// Enum integer values cross-referenced against game.db item_template columns
// (quality, equip_type, weapon_type, armor_type, weapon_material). Ordinals 0-based.

namespace ItemDefines
{
	// The per-instance item descriptor carried in packets and inventory.
	// Field order is LOCKED (m_itemId first, brace-initializable from a bare id: {itemId}).
	struct ItemDefinition
	{
		uint16_t m_itemId = 0;     // item_template.entry (first field; {id} brace init)
		int m_affixId = 0;
		int m_affixScore = 0;
		int m_enchantLvl = 0;
		int m_durability = 0;
		bool m_soulbound = false;
		int m_gem1 = 0;
		int m_gem2 = 0;
		int m_gem3 = 0;
		int m_gem4 = 0;

		ItemDefinition() = default;
		ItemDefinition(const ItemDefinition&) = default;
		ItemDefinition& operator=(const ItemDefinition&) = default;

		// The client frequently assigns a bare item id (int / uint16) to an
		// ItemDefinition (e.g. packet.m_itemId = getEntry()). Provide implicit
		// construction from an integral id; assignment from an id then goes through
		// this converting ctor + the defaulted copy-assignment. (No dedicated
		// operator=(int) — that would make `def = {}` ambiguous.)
		ItemDefinition(int id) : m_itemId(static_cast<uint16_t>(id)) {}

		// operator==/!= compare by item id ONLY (VendorNpc + others rely on entry matching).
		bool operator==(const ItemDefinition& o) const { return m_itemId == o.m_itemId; }
		bool operator!=(const ItemDefinition& o) const { return m_itemId != o.m_itemId; }

		// Exact value-equality across ALL fields. ItemIcon::setItemDef uses this to detect socket/enchant/
		// durability changes that keep the same m_itemId — an entry-only compare hid live gem fills (the
		// new gem only showed after relog rebuilt the icon).
		bool equalsExact(const ItemDefinition& o) const {
			return m_itemId == o.m_itemId && m_affixId == o.m_affixId && m_affixScore == o.m_affixScore
			    && m_enchantLvl == o.m_enchantLvl && m_durability == o.m_durability && m_soulbound == o.m_soulbound
			    && m_gem1 == o.m_gem1 && m_gem2 == o.m_gem2 && m_gem3 == o.m_gem3 && m_gem4 == o.m_gem4;
		}
	};

	// item_template.quality DB values 0..6 (ordinal). Client uses QualityLv0..QualityLv5 (+ switch cases).
	enum Quality
	{
		QualityLv0 = 0, // DB-confirmed (poor)
		QualityLv1 = 1, // DB-confirmed
		QualityLv2 = 2, // DB-confirmed
		QualityLv3 = 3, // DB-confirmed
		QualityLv4 = 4, // DB-confirmed
		QualityLv5 = 5, // DB-confirmed
		QualityLv6 = 6, // DB-confirmed (item_template.quality has value 6; rarity name TODO — distinct from
		                // the crafting-grade set Normal/Fine/Superior/Exquisite/Pristine in dump_client FUN_0048d940)
		NumQualities = 7
	};

	// m_affixScore tier (VERIFIED original client FUN_0048d940 — the equip-line material prefix).
	enum AffixScore
	{
		AffixNormal    = 1,
		AffixFine      = 2,
		AffixSuperior  = 3,
		AffixExquisite = 4,
		AffixPristine  = 5,
	};

	// item_template/affix_template stat_typeN — a 1-based id space (VERIFIED original client FUN_00436080).
	// This is NOT UnitDefines::Stat numbering; convert via ItemDefiner::statFromTypeId. Used for the
	// affix scaling switch (per-category multipliers) so those cases are named, not magic.
	enum StatTypeId
	{
		Sti_Mana = 1, Sti_Health, Sti_ArmorValue, Sti_Strength, Sti_Agility, Sti_Willpower,
		Sti_Intelligence, Sti_Courage, Sti_Regeneration, Sti_Meditate, Sti_WeaponValue,
		Sti_MeleeSpeed, Sti_RangedWeaponValue, Sti_RangedSpeed, Sti_MeleeCritical, Sti_RangedCritical,
		Sti_SpellCritical, Sti_DodgeRating, Sti_BlockRating, Sti_ParryRating, Sti_ResistFrost,
		Sti_ResistFire, Sti_ResistShadow, Sti_ResistHoly, Sti_Bartering, Sti_Lockpicking,
		// id 27 (0x1b) is skipped by the weapon-skill block (a hidden stat); skills start at 28 (VERIFIED
		// FUN_00436080 case labels: Staves=0x1c, Maces=0x1d, Swords=0x1f, Daggers=0x21, Wands=0x22, Shields=0x23).
		Sti_Staves = 28, Sti_Maces, Sti_Axes, Sti_Swords, Sti_Ranged, Sti_Daggers, Sti_Wands, Sti_Shields,
	};

	// item_template.equip_type (verified vs sample item names): 1 Cap/helm, 2 Choker/neck,
	// 3 Breastplate/chest, 4 Belt, 5 Britches/legs, 6 Boots/feet, 7 Gauntlets/hands, 8 Band/ring,
	// 9 Axe/weapon(main hand), 10 Barricade/offhand-shield, 11 Dragonslayer/ranged. 0 = non-wearable.
	enum EquipType
	{
		NoEquipType = 0,
		Helm     = 1,
		Necklace = 2,
		Chest    = 3,
		Belt     = 4,
		Legs     = 5,
		Feet     = 6,
		Hands    = 7,
		Ring     = 8,
		Weapon   = 9,
		Shield   = 10,
		Ranged   = 11,
		NumEquipTypes = 12
	};

	// item_template.weapon_type (verified vs sample names): 1 Axe, 2 Dragonslayer(ranged/bow),
	// 3 Hammer/mace, 4 Blade/sword, 5 Staff, 6 Combat Dagger, 7 Magic Rod/wand. 0 = non-weapon.
	enum WeaponType
	{
		NoWeapon = 0,
		Axe    = 1,
		Bow    = 2,
		Mace   = 3,
		Sword  = 4,
		Staff  = 5,
		Dagger = 6,
		Wand   = 7,
		NumWeaponTypes = 8
	};

	// item_template.armor_type 0..15. DEFINITIVE from client armorTypeName switch (dump_client FUN_0048e030).
	enum ArmorType
	{
		NullArmor  = 0,
		Cotton     = 1,
		Leather    = 2,
		Brigandine = 3,
		Gambeson   = 4,
		Bronze     = 5,
		Steel      = 6,
		Mythril    = 7,
		Emerald    = 8,
		Crystal    = 9,
		Diamond    = 10,
		Titanium   = 11,
		Linen      = 12,
		Wool       = 13,
		Silk       = 14,
		Spiritsilk = 15,
		NumArmorTypes = 16
	};

	// item_template.weapon_material 1..14 (0=none). DEFINITIVE from client weaponMaterialType switch
	// (dump_client FUN_0048dce0): metals 1..7, woods 8..14.
	enum WeaponMaterial
	{
		Wep_None = 0,
		Wep_Bronze = 1,
		Wep_Steel = 2,
		Wep_Mythril = 3,
		Wep_Emerald = 4,
		Wep_Crystal = 5,
		Wep_Diamond = 6,
		Wep_Titanium = 7,
		Wep_Aspen = 8,
		Wep_Elm = 9,          // 3-char wood @0x631cd4, unconfirmed exact spelling (Elm/Yew/Ash)
		Wep_Cherry = 10,
		Wep_BlackWalnut = 11,
		Wep_WhiteOak = 12,
		Wep_HardMaple = 13,
		Wep_Hickory = 14,
		NumWeaponMaterials = 15
	};

	// Visual armor model categories (client rendering). Values TODO verify.
	enum ArmorModels
	{
		ModelCloth = 0,   // TODO verify
		ModelLeather = 1, // TODO verify
		ModelMail = 2,    // TODO verify
		ModelPlate = 3,   // TODO verify
		ModelMage = 4,    // TODO verify
		ModelMageAlt1 = 5,// TODO verify
		ModelMageAlt2 = 6 // TODO verify
	};

	// Armor styling buckets used by the client tooltip/inventory. Values TODO verify.
	enum ArmorStyles
	{
		NullArmorStyle = 0,    // TODO verify
		ArmorCasterCloth = 1,  // TODO verify
		ArmorHeavy = 2         // TODO verify
	};

	// item_template.flags bitmask. DEFINITIVE from ItemTemplateEditor checkbox order (dump_client @0x144591)
	// + DB (Tome=flags 2=Skillbook). Editor creates checkboxes in bit order.
	enum ItemFlags
	{
		ItemFlag_QuestItem = 0x01,
		ItemFlag_Skillbook = 0x02,
		ItemFlag_GoldValueScales = 0x04
	};

	// Static/special item ids referenced by name in the client.
	enum StaticItems
	{
		GoldItem = 24 // item_template entry 24 "Gold" (icon_world_kina.png); was 1 = "Minor Life Potion" → gold showed a potion icon
	};
}

// ---------------------------------------------------------------------------
// ItemFunctions  (reconstructed helper namespace)
//
// Pure display-string mappers over the ItemDefines enums. Reconstructed from
// client call sites (ItemIcon.cpp, ItemTemplateEditor.cpp, SpellTemplateEditor.cpp,
// VendorRandDbEntryEditor.cpp). Every consumer transitively includes this header
// (via GameIcon.h -> ItemDefines.h), so defining the namespace here makes the
// helpers visible at all call sites without editing the client .cpp files.
//
//   qualityName(ItemDefines::Quality)            -> std::string
//   equipTypeName(ItemDefines::EquipType)        -> std::string
//   weaponTypeName(ItemDefines::WeaponType)      -> std::string
//   armorTypeName(ItemDefines::ArmorType)        -> std::string
//   weaponMaterialType(ItemDefines::WeaponMaterial) -> std::string
//   affixScoreName(int affixScore)               -> std::string  (used as: name + " " + str)
// ---------------------------------------------------------------------------
#include <string>

namespace ItemFunctions
{
	// VERIFIED from original client (formItemTitle / FUN_00494690): 1-based rarity names.
	// quality 0 (and out of range) = no prefix.
	inline std::string qualityName(ItemDefines::Quality quality)
	{
		switch (quality)
		{
		case ItemDefines::Quality::QualityLv1: return "Junk";
		case ItemDefines::Quality::QualityLv2: return "Normal";
		case ItemDefines::Quality::QualityLv3: return "Radiant";
		case ItemDefines::Quality::QualityLv4: return "Blessed";
		case ItemDefines::Quality::QualityLv5: return "Holy";
		case ItemDefines::Quality::QualityLv6: return "Godly";
		default: return ""; // QualityLv0 / out-of-range = no title prefix
		}
	}

	inline std::string equipTypeName(ItemDefines::EquipType equipType)
	{
		switch (equipType)
		{
		case ItemDefines::EquipType::Weapon:   return "Weapon";
		case ItemDefines::EquipType::Shield:   return "Shield";
		case ItemDefines::EquipType::Ranged:   return "Ranged";
		case ItemDefines::EquipType::Helm:     return "Helm";
		case ItemDefines::EquipType::Necklace: return "Necklace";
		case ItemDefines::EquipType::Chest:    return "Chest";
		case ItemDefines::EquipType::Belt:     return "Belt";
		case ItemDefines::EquipType::Legs:     return "Legs";
		case ItemDefines::EquipType::Feet:     return "Feet";
		case ItemDefines::EquipType::Hands:    return "Hands";
		case ItemDefines::EquipType::Ring:     return "Ring";
		default: break;
		}
		return "Unknown";
	}

	inline std::string weaponTypeName(ItemDefines::WeaponType weaponType)
	{
		switch (weaponType)
		{
		case ItemDefines::WeaponType::Sword:  return "Sword";
		case ItemDefines::WeaponType::Axe:    return "Axe";
		case ItemDefines::WeaponType::Mace:   return "Mace";
		case ItemDefines::WeaponType::Dagger: return "Dagger";
		case ItemDefines::WeaponType::Staff:  return "Staff";
		case ItemDefines::WeaponType::Wand:   return "Wand";
		case ItemDefines::WeaponType::Bow:    return "Bow";
		default: break;
		}
		return "Unknown";
	}

	inline std::string armorTypeName(ItemDefines::ArmorType armorType)
	{
		switch (armorType)
		{
		case ItemDefines::ArmorType::Cotton:     return "Cotton";
		case ItemDefines::ArmorType::Linen:      return "Linen";
		case ItemDefines::ArmorType::Wool:       return "Wool";
		case ItemDefines::ArmorType::Silk:       return "Silk";
		case ItemDefines::ArmorType::Spiritsilk: return "Spiritsilk";
		case ItemDefines::ArmorType::Leather:    return "Leather";
		case ItemDefines::ArmorType::Gambeson:   return "Gambeson";
		case ItemDefines::ArmorType::Brigandine: return "Brigandine";
		case ItemDefines::ArmorType::Bronze:     return "Bronze";
		case ItemDefines::ArmorType::Steel:      return "Steel";
		case ItemDefines::ArmorType::Mythril:    return "Mythril";
		case ItemDefines::ArmorType::Titanium:   return "Titanium";
		case ItemDefines::ArmorType::Crystal:    return "Crystal";
		case ItemDefines::ArmorType::Emerald:    return "Emerald";
		case ItemDefines::ArmorType::Diamond:    return "Diamond";
		default: break;
		}
		return "Unknown";
	}

	inline std::string weaponMaterialType(ItemDefines::WeaponMaterial material)
	{
		switch (material)
		{
		case ItemDefines::WeaponMaterial::Wep_Aspen:       return "Aspen";
		case ItemDefines::WeaponMaterial::Wep_Elm:         return "Elm";
		case ItemDefines::WeaponMaterial::Wep_HardMaple:   return "Hard Maple";
		case ItemDefines::WeaponMaterial::Wep_Cherry:      return "Cherry";
		case ItemDefines::WeaponMaterial::Wep_BlackWalnut: return "Black Walnut";
		case ItemDefines::WeaponMaterial::Wep_Hickory:     return "Hickory";
		case ItemDefines::WeaponMaterial::Wep_WhiteOak:    return "White Oak";
		case ItemDefines::WeaponMaterial::Wep_Bronze:      return "Bronze";
		case ItemDefines::WeaponMaterial::Wep_Steel:       return "Steel";
		case ItemDefines::WeaponMaterial::Wep_Mythril:     return "Mythril";
		case ItemDefines::WeaponMaterial::Wep_Titanium:    return "Titanium";
		case ItemDefines::WeaponMaterial::Wep_Crystal:     return "Crystal";
		case ItemDefines::WeaponMaterial::Wep_Emerald:     return "Emerald";
		case ItemDefines::WeaponMaterial::Wep_Diamond:     return "Diamond";
		default: break;
		}
		return "Unknown";
	}

	// Used as: affixScoreName(m_itemDef.m_affixScore) + " " + str   (ItemIcon.cpp:306)
	// VERIFIED from original client (FUN_0048d940): direct 1-based index, NOT thresholds.
	inline std::string affixScoreName(int affixScore)
	{
		switch (static_cast<ItemDefines::AffixScore>(affixScore))
		{
		case ItemDefines::AffixScore::AffixNormal:    return "Normal";
		case ItemDefines::AffixScore::AffixFine:      return "Fine";
		case ItemDefines::AffixScore::AffixSuperior:  return "Superior";
		case ItemDefines::AffixScore::AffixExquisite: return "Exquisite";
		case ItemDefines::AffixScore::AffixPristine:  return "Pristine";
		default: return "Unknown";
		}
	}
}
