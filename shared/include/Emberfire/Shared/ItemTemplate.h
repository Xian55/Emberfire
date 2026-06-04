#pragma once
//
// ItemTemplate / ItemAffix — reconstructed shared item definition structs.
// Loaded from the game DB by ItemDefiner. The client reads base + affix stats to
// build item tooltips (ItemIcon.cpp, ItemTemplateEditor.cpp).
//
#include <map>
#include <string>
#include <vector>

#include "UnitDefines.h"   // UnitDefines::Stat (written by enum agent)
#include "ItemAffix.h"     // canonical ItemAffix (do not redefine here)

// One row of item_template plus its precomputed base stat block.
struct ItemTemplate
{
    int m_entry{0};
    std::string m_name;

    // Explicit bonus stats (item_template.stat_typeN/stat_valueN), keyed by UnitDefines::Stat.
    // (stat_type is a 1-based spend-id; converted at load via ItemDefiner::statFromTypeId.)
    std::map<UnitDefines::Stat, int> m_baseStats;

    int m_requiredLevel{0};
    int m_numSockets{0};
    int m_quality{0};
    // Drive the armor-value + affix-scaling formulas (see ItemDefiner).
    int m_armorType{0};   // item_template.armor_type
    int m_equipType{0};   // item_template.equip_type
    int m_weaponType{0};  // item_template.weapon_type
    int m_itemLevel{0};   // item_template.item_level
};

// ItemAffix is defined canonically in ItemAffix.h (included above).
