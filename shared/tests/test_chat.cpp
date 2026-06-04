// Locks the C2S Chat (op17) wire layout. Ground truth = original Steam client capture
// (tools/capture/session-2026-06-04T...-conn2-v1189): channel:u8, text:cstr, targetName:cstr, then a
// 12-byte item-link blob (zeroed when no item). Omitting the 12-byte blob made every chat send 12 bytes
// short -> the server underflowed reading the link and dropped the connection. Keep this test green.
#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "GamePacketServer.h"  // pulls in GamePackets_Stubs.h (GP_Client_ChatMsg) + Opcode (global ns)

TEST(ClientChatMsg, SayMatchesSteamCaptureBytes) {
    GP_Client_ChatMsg p;
    p.m_channelId = 0;          // Say
    p.m_text = "gooday";
    p.m_targetName = "";        // empty for non-whisper

    // body = opcode(u16 LE) + payload; sendPacket() prepends the 4-byte length frame separately.
    const std::vector<std::uint8_t> expected = {
        0x11, 0x00,                                     // opcode 17 (Client_ChatMsg), little-endian
        0x00,                                           // channel = Say
        0x67, 0x6f, 0x6f, 0x64, 0x61, 0x79, 0x00,       // "gooday\0"
        0x00,                                           // targetName "" (just the NUL)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             // 12-byte item-link blob (zeroed)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    EXPECT_EQ(p.build(StlBuffer{}).raw(), expected);
    EXPECT_EQ(p.build(StlBuffer{}).raw().size(), 23u); // matches conn2 op17 "gooday" frame body length
}

TEST(ClientChatMsg, OpcodeIs17) {
    EXPECT_EQ(Opcode::Client_ChatMsg, 17);
}
