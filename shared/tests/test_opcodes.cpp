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
