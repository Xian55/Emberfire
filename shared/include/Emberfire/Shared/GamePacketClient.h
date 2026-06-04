#pragma once
#include "GamePacketServer.h" // Opcode enum + GamePacket base + StlBuffer
// Decoded client packets (CONFIRMED from capture). build(StlBuffer&&) writes [opcode][fields]; the length
// prefix is added by SfSocket::sendPacket. Pattern: `sConnector->sendPacket(pk.build(StlBuffer{}))`.

struct GP_Client_Authenticate : GamePacket {
    std::string m_userToken; std::int32_t m_build = 0; std::string m_fingerprint;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_Authenticate) << m_userToken << m_build << m_fingerprint; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_EnterWorld : GamePacket {
    std::uint32_t m_playerGuid = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_EnterWorld) << m_playerGuid; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_CharCreate : GamePacket {
    std::uint8_t m_gender = 0, m_portrait = 0; std::string m_name; std::uint8_t m_classId = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_CharCreate) << m_gender << m_portrait << m_name << m_classId; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_RequestMove : GamePacket {
    float m_destX = 0, m_destY = 0; std::uint8_t m_wasd = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_RequestMove) << m_destX << m_destY << m_wasd; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Client_RequestStop : GamePacket {
    float m_myX = 0, m_myY = 0;
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Client_RequestStop) << m_myX << m_myY; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};
struct GP_Mutual_Ping : GamePacket {
    std::uint64_t m_time = 0;
    void unpack(StlBuffer& d) { d >> m_time; }
    StlBuffer& build(StlBuffer&& b) { b << std::uint16_t(Mutual_Ping) << m_time; m_buf = std::move(b); return m_buf; }
    StlBuffer m_buf;
};

// The remaining ~65 GP_Client_* and ~70 GP_Server_* classes are generated as stubs in
// GamePackets_Stubs.h (field orders from build/recon/SHARED_INTERFACE.md). // TODO: confirm each body.
