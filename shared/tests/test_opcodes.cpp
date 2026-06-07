// Opcode values + the UseItem "no target" sentinel — both wire-confirmed and previously mis-set.
#include <gtest/gtest.h>
#include "GamePacketServer.h"

TEST(Opcodes, WireConfirmedValues) {
    EXPECT_EQ(Opcode::Client_UseItem,       11);
    EXPECT_EQ(Opcode::Server_EquipItem,     86);   // live equip/socket broadcast
    EXPECT_EQ(Opcode::Server_SocketResult,  121);  // was mis-set to 147 (arena); op121 is SocketResult
}

TEST(UseItem, UnusedTargetSlotDefaultsTo0xFF) {
    // Socketing an equipped item sends tgtEq=<slot>, tgtInv must be 0xff (not 0 = "inv slot 0").
    GP_Client_UseItem pkt;
    EXPECT_EQ(pkt.m_target_InvSlot,   255);
    EXPECT_EQ(pkt.m_target_EquipSlot, 255);
}

TEST(LevelUp, SpendIdIsEnumIndexNoHealthManaSwap) {
    // op40 statId == the stat's enum/var index for EVERY stat. There is NO Health/Mana swap: an earlier
    // swap (Health=1/Mana=2) made an invested Health point land on Mana on the live original server.
    using S = UnitDefines::Stat;
    EXPECT_EQ(GP_Client_LevelUp::spendId(S::Mana),       1);   // var 0x0f
    EXPECT_EQ(GP_Client_LevelUp::spendId(S::Health),     2);   // var 0x10 (NOT 1)
    EXPECT_EQ(GP_Client_LevelUp::spendId(S::ArmorValue), 3);
    EXPECT_EQ(GP_Client_LevelUp::spendId(S::Strength),   4);
    EXPECT_EQ(GP_Client_LevelUp::spendId(S::Bartering),  25);
}

TEST(LevelUp, HealthSpendWritesStatId2) {
    // Wire: [op:u16][statCount:u16][statId:u16,val:u16]*[spellCount:u16]; investing Health => statId 2.
    GP_Client_LevelUp pk;
    pk.m_statInvestments[UnitDefines::Stat::Health] = 3;
    pk.build(StlBuffer{});
    const std::vector<std::uint8_t>& w = pk.m_buf.raw();
    ASSERT_EQ(w.size(), 10u);
    auto u16 = [&](std::size_t i) { return std::uint16_t(w[i] | (w[i + 1] << 8)); };
    EXPECT_EQ(u16(0), std::uint16_t(Opcode::Client_LevelUp));
    EXPECT_EQ(u16(2), 1);   // statCount
    EXPECT_EQ(u16(4), 2);   // statId = Health = 2 (the bug sent 1 = Mana)
    EXPECT_EQ(u16(6), 3);   // points
    EXPECT_EQ(u16(8), 0);   // spellCount
}
