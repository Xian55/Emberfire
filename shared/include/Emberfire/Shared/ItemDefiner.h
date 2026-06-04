#pragma once
//
// ItemDefiner — reconstructed shared singleton that loads item templates + affixes
// from the game DB and computes per-item stat blocks for the client tooltips.
//
// Access via the `sItemDefiner` macro (hand-rolled Meyers singleton, matching the
// other Shared singletons described in SHARED_INTERFACE.md §B).
//
// Client call sites (ItemIcon.cpp / ItemTemplateEditor.cpp):
//   sItemDefiner->baseItemStats(entry, requiredLevel, enchantLvl)
//   sItemDefiner->crunchItemStats(entry, affixId, affixScore, requiredLevel, enchantLvl)
//   sItemDefiner->getAffix(affixId)
//   sItemDefiner->loadItemTemplates(db) / loadAffixes(db) / reloadItemEntry(db, entry)
//
#include <map>
#include <memory>
#include <string>
#include <algorithm>         // std::max

#include "UnitDefines.h"     // UnitDefines::Stat
#include "ItemDefines.h"     // ItemDefines::StatTypeId / ArmorType / EquipType / Quality
#include "ItemTemplate.h"
#include "../SqlConnector/SqlConnector.h"  // SqlConnector::query + QueryResult/Field

class ItemDefiner
{
public:
    static ItemDefiner* instance()
    {
        static ItemDefiner inst;
        return &inst;
    }

    // Convert a 1-based item/affix stat_type id (ItemDefines::StatTypeId) to UnitDefines::Stat.
    // VERIFIED from the original client name function FUN_00436080. The StatTypeId space differs from
    // UnitDefines::Stat (it omits MeleeValue/Fortitude/Perception), so this is an explicit named map.
    static UnitDefines::Stat statFromTypeId(int typeId)
    {
        using S = UnitDefines::Stat;
        switch (static_cast<ItemDefines::StatTypeId>(typeId))
        {
        case ItemDefines::Sti_Mana:              return S::Mana;
        case ItemDefines::Sti_Health:            return S::Health;
        case ItemDefines::Sti_ArmorValue:        return S::ArmorValue;
        case ItemDefines::Sti_Strength:          return S::Strength;
        case ItemDefines::Sti_Agility:           return S::Agility;
        case ItemDefines::Sti_Willpower:         return S::Willpower;
        case ItemDefines::Sti_Intelligence:      return S::Intelligence;
        case ItemDefines::Sti_Courage:           return S::Courage;
        case ItemDefines::Sti_Regeneration:      return S::Regeneration;
        case ItemDefines::Sti_Meditate:          return S::Meditate;
        case ItemDefines::Sti_WeaponValue:       return S::WeaponValue;
        case ItemDefines::Sti_MeleeSpeed:        return S::MeleeSpeed;
        case ItemDefines::Sti_RangedWeaponValue: return S::RangedValue;
        case ItemDefines::Sti_RangedSpeed:       return S::RangedSpeed;
        case ItemDefines::Sti_MeleeCritical:     return S::MeleeCritical;
        case ItemDefines::Sti_RangedCritical:    return S::RangedCritical;
        case ItemDefines::Sti_SpellCritical:     return S::SpellCritical;
        case ItemDefines::Sti_DodgeRating:       return S::DodgeRating;
        case ItemDefines::Sti_BlockRating:       return S::BlockRating;
        case ItemDefines::Sti_ParryRating:       return S::ParryRating;
        case ItemDefines::Sti_ResistFrost:       return S::ResistFrost;
        case ItemDefines::Sti_ResistFire:        return S::ResistFire;
        case ItemDefines::Sti_ResistShadow:      return S::ResistShadow;
        case ItemDefines::Sti_ResistHoly:        return S::ResistHoly;
        case ItemDefines::Sti_Bartering:         return S::Bartering;
        case ItemDefines::Sti_Lockpicking:       return S::Lockpicking;
        case ItemDefines::Sti_Staves:            return S::StaffSkill;
        case ItemDefines::Sti_Maces:             return S::MaceSkill;
        case ItemDefines::Sti_Axes:              return S::AxesSkill;
        case ItemDefines::Sti_Swords:            return S::SwordSkill;
        case ItemDefines::Sti_Ranged:            return S::RangedSkill;
        case ItemDefines::Sti_Daggers:           return S::DaggerSkill;
        case ItemDefines::Sti_Wands:             return S::WandSkill;
        case ItemDefines::Sti_Shields:           return S::ShieldSkill;
        default:                                 return UnitDefines::NullStat;
        }
    }

    // ---- loaders ----
    // Explicit bonus stats (item_template.stat_typeN/stat_valueN) + the material/type fields the
    // armor-value + affix-scaling formulas need. stat_type is a StatTypeId (converted via statFromTypeId).
    void loadItemTemplates(std::shared_ptr<SqlConnector> db)
    {
        m_templates.clear();
        if (!db) return;
        std::string sql = "SELECT entry";
        for (int i = 1; i <= 10; ++i) sql += ",stat_type" + std::to_string(i) + ",stat_value" + std::to_string(i);
        sql += ",required_level,num_sockets,quality,name,armor_type,equip_type,weapon_type,item_level FROM item_template";
        auto r = db->query(sql);
        if (!r) return;
        for (auto& row : r->fetchData())
        {
            if (row.size() < 29) continue;
            ItemTemplate t;
            t.m_entry = row[0].getInt32();
            for (int i = 0; i < 10; ++i)
            {
                int sv = row[2 + i * 2].getInt32();
                int typeId = row[1 + i * 2].getInt32();
                if (sv != 0 && typeId != 0)
                    t.m_baseStats[statFromTypeId(typeId)] += sv;
            }
            t.m_requiredLevel = row[21].getInt32();
            t.m_numSockets    = row[22].getInt32();
            t.m_quality       = row[23].getInt32();
            t.m_name          = row[24].getString();
            t.m_armorType     = row[25].getInt32();
            t.m_equipType     = row[26].getInt32();
            t.m_weaponType    = row[27].getInt32();
            t.m_itemLevel     = row[28].getInt32();
            m_templates.emplace(t.m_entry, std::move(t));
        }
    }
    void loadAffixes(std::shared_ptr<SqlConnector> db)
    {
        m_affixes.clear();
        if (!db) return;
        std::string sql = "SELECT entry,name,name_single_noun,min_level,max_level";
        for (int i = 1; i <= 5; ++i) sql += ",stat_type" + std::to_string(i) + ",stat_value" + std::to_string(i);
        sql += " FROM affix_template";
        auto r = db->query(sql);
        if (!r) return;
        for (auto& row : r->fetchData())
        {
            if (row.size() < 15) continue;
            auto a = std::make_shared<ItemAffix>();
            a->m_entry = row[0].getInt32();
            a->m_name = row[1].getString();
            a->m_nameSingleNoun = row[2].getString();
            a->m_minLevel = row[3].getInt32();
            a->m_maxLevel = row[4].getInt32();
            for (int i = 0; i < 5; ++i)
            {
                int typeId = row[5 + i * 2].getInt32();
                float sv   = row[6 + i * 2].getFloat();   // affix_template.stat_value is REAL (e.g. 1.7, 0.333)
                if (typeId != 0 && sv != 0.f)
                    a->addStat(typeId, statFromTypeId(typeId), sv);
            }
            m_affixes.emplace(a->m_entry, std::move(a));
        }
    }
    void reloadItemEntry(std::shared_ptr<SqlConnector> /*db*/, const int /*entry*/) {}

    // ---- queries ----
    // Quality multiplier (VERIFIED FUN_00558300/FUN_005587e0): Blessed 1.33, Holy 1.66, Godly 2.0, else 1.0.
    static float qualityMult(const int quality)
    {
        switch (static_cast<ItemDefines::Quality>(quality))
        {
        case ItemDefines::Quality::QualityLv4: return 1.33f; // Blessed
        case ItemDefines::Quality::QualityLv5: return 1.66f; // Holy
        case ItemDefines::Quality::QualityLv6: return 2.00f; // Godly
        default:                               return 1.00f;
        }
    }

    // Base (un-affixed) stats: explicit bonuses + the derived Armor Value.
    // Armor Value (VERIFIED original client FUN_00558300): armorSlotRemap[armor_type] * (Chest?1.5:1) *
    // qualityMult, floored to int with a minimum of 1 when the item has an armor_type.
    std::map<UnitDefines::Stat, int> baseItemStats(const int entry,
                                                   const int requiredLevel,
                                                   const int enchantLvl)
    {
        auto itr = m_templates.find(entry);
        if (itr == m_templates.end())
            return {};
        const ItemTemplate& t = itr->second;
        std::map<UnitDefines::Stat, int> out = t.m_baseStats;

        // Weapon value + speed (VERIFIED FUN_00558300 weapon branch):
        //   value = max( max(reqLevel + 0.5*enchant, 1) * 3 * typeMult * itemLevelMult * weaponQualityMult + base, 1 )
        // Bow -> RangedWeaponValue/RangedCooldown; all other weapons -> WeaponValue/MeleeCooldown. cooldown is ms.
        if (t.m_weaponType != 0)
        {
            int   cooldownMs = 2500;        // default (Wand / other)
            float base = 0.0f, typeMult = 0.75f;
            bool  ranged = false;
            switch (static_cast<ItemDefines::WeaponType>(t.m_weaponType))
            {
            case ItemDefines::WeaponType::Axe:
            case ItemDefines::WeaponType::Mace:
            case ItemDefines::WeaponType::Sword:
            case ItemDefines::WeaponType::Staff:
                cooldownMs = 2000; base = static_cast<float>(requiredLevel + t.m_quality); typeMult = 1.0f; break;
            case ItemDefines::WeaponType::Bow:
                cooldownMs = 3000; base = static_cast<float>(requiredLevel + t.m_quality); typeMult = 1.0f; ranged = true; break;
            case ItemDefines::WeaponType::Dagger:
                cooldownMs = 1500; base = static_cast<float>(requiredLevel); typeMult = 0.5f; break;
            default: break; // Wand etc.
            }
            float ilMult = 1.0f;
            switch (t.m_itemLevel)
            {
            case 2: case 9:  ilMult = 1.1f; break;
            case 3: case 10: ilMult = 1.2f; break;
            case 4: case 11: ilMult = 1.3f; break;
            case 5: case 12: ilMult = 1.4f; break;
            case 6: case 13: ilMult = 1.5f; break;
            case 7: case 14: ilMult = 1.6f; break;
            default: break;
            }
            float wq = 1.0f; // weapon quality multiplier (distinct from armor's qualityMult)
            switch (static_cast<ItemDefines::Quality>(t.m_quality))
            {
            case ItemDefines::Quality::QualityLv3: wq = 1.125f; break;
            case ItemDefines::Quality::QualityLv4: wq = 1.25f;  break;
            case ItemDefines::Quality::QualityLv5: wq = 1.375f; break;
            case ItemDefines::Quality::QualityLv6: wq = 1.5f;   break;
            default: break;
            }
            const float L = std::max(static_cast<float>(requiredLevel) + 0.5f * static_cast<float>(enchantLvl), 1.0f);
            const int value = static_cast<int>(std::max(L * 3.0f * typeMult * ilMult * wq + base, 1.0f));
            if (ranged)
            {
                out[UnitDefines::Stat::RangedWeaponValue] = value;
                out[UnitDefines::Stat::RangedCooldown]    = cooldownMs;
            }
            else
            {
                out[UnitDefines::Stat::WeaponValue]   = value;
                out[UnitDefines::Stat::MeleeCooldown] = cooldownMs;
            }
        }

        // armorSlotRemap (VERIFIED FUN_00558300 jump table): armor_type -> slot weight.
        int slot = 0;
        switch (static_cast<ItemDefines::ArmorType>(t.m_armorType))
        {
        case ItemDefines::ArmorType::Cotton:     slot = 1;  break;
        case ItemDefines::ArmorType::Leather:    slot = 6;  break;
        case ItemDefines::ArmorType::Brigandine: slot = 7;  break;
        case ItemDefines::ArmorType::Gambeson:   slot = 8;  break;
        case ItemDefines::ArmorType::Bronze:     slot = 9;  break;
        case ItemDefines::ArmorType::Steel:      slot = 10; break;
        case ItemDefines::ArmorType::Mythril:    slot = 11; break;
        case ItemDefines::ArmorType::Emerald:    slot = 12; break;
        case ItemDefines::ArmorType::Crystal:    slot = 13; break;
        case ItemDefines::ArmorType::Diamond:    slot = 14; break;
        case ItemDefines::ArmorType::Titanium:   slot = 15; break;
        case ItemDefines::ArmorType::Linen:      slot = 2;  break;
        case ItemDefines::ArmorType::Wool:       slot = 3;  break;
        case ItemDefines::ArmorType::Silk:       slot = 4;  break;
        case ItemDefines::ArmorType::Spiritsilk: slot = 5;  break;
        default: break; // NullArmor / non-armor item -> no armor value
        }
        if (slot > 0)
        {
            const float eMult = (static_cast<ItemDefines::EquipType>(t.m_equipType) == ItemDefines::EquipType::Chest) ? 1.5f : 1.0f;
            const float v = static_cast<float>(slot) * eMult * qualityMult(t.m_quality);
            const int armor = (v > 0.f && v < 1.0f) ? 1 : static_cast<int>(v);
            if (armor > 0)
                out[UnitDefines::Stat::ArmorValue] = armor;
        }
        return out;
    }

    // Bonus stats shown as "+N <stat>" / "Equip:" lines = explicit item stats + affix(scaled). Affix scaling
    // VERIFIED from FUN_005587e0:
    //   per stat:  scaled = max( value * positionalMult(typeId) * levelF * qualityMult * equipMult, 1 )
    //   where levelF = max(max(reqLevel + 0.5*enchant, 1) * 0.5, 1); then an affixScore bonus; floor to int.
    // NOTE: does NOT include the derived Armor/Weapon Value (those are header-only, from baseItemStats);
    // including them here would double-render them as bonus lines (FUN_005587e0 is separate from FUN_00558300).
    std::map<UnitDefines::Stat, int> crunchItemStats(const int entry,
                                                     const int affixId,
                                                     const int affixScore,
                                                     const int requiredLevel,
                                                     const int enchantLvl)
    {
        std::map<UnitDefines::Stat, int> out;
        int equipType = 0, quality = 0;
        auto titr = m_templates.find(entry);
        if (titr != m_templates.end())
        {
            out       = titr->second.m_baseStats; // explicit item bonus stats only
            equipType = titr->second.m_equipType;
            quality   = titr->second.m_quality;
        }

        auto affix = getAffix(affixId);
        if (!affix)
            return out;

        const float L      = static_cast<float>(requiredLevel) + 0.5f * static_cast<float>(enchantLvl);
        const float levelF = std::max(std::max(L, 1.0f) * 0.5f, 1.0f);
        const float qMult  = qualityMult(quality);

        // equip-slot multiplier (VERIFIED FUN_005587e0): helm/legs/feet/hands/shield 0.65; neck/belt/ring 0.5.
        float eMult = 1.0f;
        switch (static_cast<ItemDefines::EquipType>(equipType))
        {
        case ItemDefines::EquipType::Helm:
        case ItemDefines::EquipType::Legs:
        case ItemDefines::EquipType::Feet:
        case ItemDefines::EquipType::Hands:
        case ItemDefines::EquipType::Shield:   eMult = 0.65f; break;
        case ItemDefines::EquipType::Necklace:
        case ItemDefines::EquipType::Belt:
        case ItemDefines::EquipType::Ring:     eMult = 0.50f; break;
        default: break;
        }

        std::map<UnitDefines::Stat, float> acc;
        const auto& mods = affix->stats();
        for (const auto& m : mods)
        {
            float v = m.value;
            if (v <= 0.f)
                continue;
            // positional multiplier by stat category (VERIFIED FUN_005587e0).
            switch (static_cast<ItemDefines::StatTypeId>(m.typeId))
            {
            case ItemDefines::Sti_Mana:
            case ItemDefines::Sti_Health:        v *= 5.0f; break;
            case ItemDefines::Sti_Regeneration:
            case ItemDefines::Sti_Meditate:      v *= 0.5f; break;
            case ItemDefines::Sti_MeleeCritical:
            case ItemDefines::Sti_RangedCritical:
            case ItemDefines::Sti_SpellCritical:
            case ItemDefines::Sti_BlockRating:
            case ItemDefines::Sti_ResistFrost:
            case ItemDefines::Sti_ResistFire:
            case ItemDefines::Sti_ResistShadow:
            case ItemDefines::Sti_ResistHoly:    v = (v / 10.0f) * L; break;
            case ItemDefines::Sti_DodgeRating:   v = (v / 15.0f) * L; break;
            case ItemDefines::Sti_Staves:
            case ItemDefines::Sti_Maces:
            case ItemDefines::Sti_Axes:
            case ItemDefines::Sti_Swords:
            case ItemDefines::Sti_Ranged:
            case ItemDefines::Sti_Daggers:
            case ItemDefines::Sti_Wands:
            case ItemDefines::Sti_Shields:       v *= 0.35f; break;
            default: break;
            }
            acc[m.stat] += std::max(v * levelF * qMult * eMult, 1.0f);
        }

        // affixScore bonus (VERIFIED FUN_005587e0): score >= max -> +1 to every stat; otherwise, when
        // score < stat-count, +1 to one stat at index floor(numStats * score / max).
        const int kMaxAffixScore = ItemDefines::AffixScore::AffixPristine; // 5
        const int numStats = static_cast<int>(mods.size());
        if (affixScore >= kMaxAffixScore)
        {
            for (const auto& m : mods) acc[m.stat] += 1.0f;
        }
        else if (affixScore < numStats)
        {
            const int idx = static_cast<int>(static_cast<float>(numStats) *
                                             (static_cast<float>(affixScore) / static_cast<float>(kMaxAffixScore)));
            if (idx >= 0 && idx < numStats)
                acc[mods[idx].stat] += 1.0f;
        }

        for (const auto& kv : acc)
        {
            const float fv = kv.second;
            const int iv = (fv > 0.f && fv < 1.0f) ? 1 : static_cast<int>(fv);
            if (iv != 0)
                out[kv.first] += iv;
        }
        return out;
    }

    // Returns the affix definition for affixId, or nullptr.
    std::shared_ptr<ItemAffix> getAffix(const int affixId)
    {
        auto itr = m_affixes.find(affixId);
        return itr == m_affixes.end() ? nullptr : itr->second;
    }

    // Test seam: inject a template/affix without a DB (used by build/test/test_item_stats.cpp).
    void putTemplateForTest(const ItemTemplate& t) { m_templates[t.m_entry] = t; }
    void putAffixForTest(const std::shared_ptr<ItemAffix>& a) { m_affixes[a->m_entry] = a; }

private:
    ItemDefiner() = default;

    std::map<int, ItemTemplate> m_templates;
    std::map<int, std::shared_ptr<ItemAffix>> m_affixes;
};

#define sItemDefiner ItemDefiner::instance()
