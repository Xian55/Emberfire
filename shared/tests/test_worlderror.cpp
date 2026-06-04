// WorldError codes — ground truth from the client's World::error switch (binary FUN_0053f530).
// These were repeatedly mis-guessed; lock them so a wrong toast can never silently regress.
#include <gtest/gtest.h>
#include "PlayerDefines.h"

using PlayerDefines::WorldError;

TEST(WorldError, BinaryConfirmedCodes) {
    EXPECT_EQ(WorldError::NotHighEnoughLevel,     40);
    EXPECT_EQ(WorldError::SpellRequirementNotMet, 41);
    EXPECT_EQ(WorldError::EnchantLimit,           42);
    EXPECT_EQ(WorldError::NoEmptySockets,         43);
    EXPECT_EQ(WorldError::NotEnoughExp,           44);
    EXPECT_EQ(WorldError::MagicSchoolLock,        45);
    EXPECT_EQ(WorldError::LockpickTooLow,         46);
    EXPECT_EQ(WorldError::WrongClass,             47);  // "You are not the right Class"
    EXPECT_EQ(WorldError::AlreadyKnowSpell,       48);
    EXPECT_EQ(WorldError::RequiresLockPicking,    49);
    EXPECT_EQ(WorldError::ItemIsSoulBound,        50);
    EXPECT_EQ(WorldError::CantInArena,            51);
    EXPECT_EQ(WorldError::CantInDungeon,          52);
}

TEST(WorldError, LowCodesUnchanged) {
    EXPECT_EQ(WorldError::SpellNotReady, 4);
    EXPECT_EQ(WorldError::LineOfSight,   6);
    EXPECT_EQ(WorldError::InvalidTarget, 9);
    EXPECT_EQ(WorldError::OutOfRange,    12);
}
