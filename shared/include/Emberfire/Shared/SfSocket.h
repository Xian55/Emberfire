#pragma once
#include <memory>
#include <vector>
#include <deque>
#include <cstdint>
#include "StlBuffer.h"
// NOTE: needs SFML 2.6 (sf::TcpSocket). Framing reverse-engineered from live capture:
//   each packet on the wire = [uint32 LE bodyLen][bodyLen bytes]  (body begins with the uint16 opcode).
#include <SFML/Network.hpp>

// Extension point the client constructs via make_shared<TcpSocketEx>().
class TcpSocketEx : public sf::TcpSocket {
public:
    using sf::TcpSocket::TcpSocket;
};

class SfSocket {
public:
    enum class Type { SocketType_ClientSide, SocketType_ServerSide };

    SfSocket(std::shared_ptr<TcpSocketEx> sock, Type type) : m_sock(std::move(sock)), m_type(type) {
        if (m_sock) m_sock->setBlocking(false);
    }

    // Pump the socket; returns false on disconnect. Reassembles whole packets into m_inbox.
    bool update() {
        if (!m_sock) return false;
        std::uint8_t tmp[16384];
        for (;;) {
            std::size_t got = 0;
            sf::Socket::Status st = m_sock->receive(tmp, sizeof(tmp), got);
            if (st == sf::Socket::Done && got > 0) {
                m_acc.insert(m_acc.end(), tmp, tmp + got);
                continue;
            }
            if (st == sf::Socket::NotReady) break;
            if (st == sf::Socket::Disconnected || st == sf::Socket::Error) { m_connected = false; return false; }
            break;
        }
        // split frames
        std::size_t off = 0;
        while (off + 4 <= m_acc.size()) {
            std::uint32_t len = m_acc[off] | (m_acc[off+1] << 8) | (m_acc[off+2] << 16) | (std::uint32_t(m_acc[off+3]) << 24);
            if (off + 4 + len > m_acc.size()) break;
            auto pkt = std::make_unique<StlBuffer>();
            for (std::size_t i = 0; i < len; ++i) (*pkt) << m_acc[off + 4 + i]; // raw copy into buffer
            // strip: caller reads opcode then eraseFront; we hand a buffer that begins at the opcode
            m_inbox.push_back(std::move(pkt));
            off += 4 + len;
        }
        if (off) m_acc.erase(m_acc.begin(), m_acc.begin() + off);
        return true;
    }

    void cancel() { m_acc.clear(); m_inbox.clear(); }

    void sendPacket(StlBuffer& outgoing) {
        if (!m_sock) return;
        const auto& body = outgoing.raw();
        std::vector<std::uint8_t> frame;
        std::uint32_t len = static_cast<std::uint32_t>(body.size());
        frame.push_back(len & 0xff); frame.push_back((len >> 8) & 0xff);
        frame.push_back((len >> 16) & 0xff); frame.push_back((len >> 24) & 0xff);
        frame.insert(frame.end(), body.begin(), body.end());
        std::size_t sent = 0;
        m_sock->send(frame.data(), frame.size(), sent); // TODO: handle partial sends / Partial status
    }

    void popReceived(std::vector<std::unique_ptr<StlBuffer>>& out) {
        while (!m_inbox.empty()) { out.push_back(std::move(m_inbox.front())); m_inbox.pop_front(); }
    }

    bool connected() const { return m_connected && m_sock != nullptr; }

private:
    std::shared_ptr<TcpSocketEx> m_sock;
    Type m_type;
    bool m_connected = true;
    std::vector<std::uint8_t> m_acc;
    std::deque<std::unique_ptr<StlBuffer>> m_inbox;
};
