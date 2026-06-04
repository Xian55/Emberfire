// Spell Attributes are a 64-bit field; the cursor/targeting bits MUST be the high bits 33/35/38,
// and the enum MUST have a 64-bit underlying type (else MSVC truncates 1ull<<35 to 0).
#include <gtest/gtest.h>
#include "SpellDefines.h"

using SpellDefines::Attributes;

TEST(Attributes, HighBitValues) {
    EXPECT_EQ(static_cast<unsigned long long>(Attributes::TargetsGround),      1ull << 33);
    EXPECT_EQ(static_cast<unsigned long long>(Attributes::TargetsItem),        1ull << 35);
    EXPECT_EQ(static_cast<unsigned long long>(Attributes::MouseoverTargeting), 1ull << 38);
}

TEST(Attributes, UnderlyingTypeIs64Bit) {
    EXPECT_EQ(sizeof(Attributes), 8u);  // must not truncate to 0
}

TEST(Attributes, MaskHasMatchesLiveSpells) {
    // gem (Rough Ruby, spell 112) attributes = 0x800000000 -> has TargetsItem (socket cursor)
    EXPECT_TRUE(Util::maskHas(0x800000000ull, Attributes::TargetsItem));
    // Sleep (spell 245) attributes = 0x40400000 -> NOT a cursor spell (self-cast)
    EXPECT_FALSE(Util::maskHas(0x40400000ull, Attributes::MouseoverTargeting));
    EXPECT_FALSE(Util::maskHas(0x40400000ull, Attributes::TargetsGround));
    // Pick Lock (spell 273) attributes = 0x4080400000 -> MouseoverTargeting (click-a-target cursor)
    EXPECT_TRUE(Util::maskHas(0x4080400000ull, Attributes::MouseoverTargeting));
}
