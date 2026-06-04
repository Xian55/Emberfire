// ItemDefinition::equalsExact must compare ALL fields (entry/affix/enchant/durability/gems), while
// operator== stays entry-only (VendorNpc relies on it). This is what makes a live gem-fill show without
// a relog: setItemDef refreshes the tooltip only when equalsExact() reports a change.
#include <gtest/gtest.h>
#include "ItemDefines.h"

using ItemDefines::ItemDefinition;

TEST(ItemDefinition, OperatorEqualsIsEntryOnly) {
    ItemDefinition a; a.m_itemId = 19426; a.m_gem1 = 0;
    ItemDefinition b; b.m_itemId = 19426; b.m_gem1 = 4;  // same item, a gem was socketed
    EXPECT_TRUE(a == b);   // entry-only -> "equal" (vendor slot matching)
}

TEST(ItemDefinition, EqualsExactComparesGems) {
    ItemDefinition a; a.m_itemId = 19426; a.m_gem1 = 0;
    ItemDefinition b; b.m_itemId = 19426; b.m_gem1 = 4;
    EXPECT_FALSE(a.equalsExact(b));   // gem changed -> NOT exactly equal -> tooltip refreshes
    EXPECT_TRUE(a.equalsExact(a));
}

TEST(ItemDefinition, EqualsExactComparesEnchantAndDurability) {
    ItemDefinition a; a.m_itemId = 100; a.m_enchantLvl = 0; a.m_durability = 40;
    ItemDefinition b = a; b.m_enchantLvl = 5;
    EXPECT_FALSE(a.equalsExact(b));
    ItemDefinition c = a; c.m_durability = 39;
    EXPECT_FALSE(a.equalsExact(c));
}
