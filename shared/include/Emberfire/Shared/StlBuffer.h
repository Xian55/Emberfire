#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <fstream>
#include <iterator>

// Reconstructed from live wire capture (see build/recon/PROTOCOL.md):
//   - little-endian, fixed width per type (u8/bool=1, u16=2, int/u32/float=4, u64/double=8)
//   - std::string is NULL-TERMINATED (no length prefix)
//   - operator>> reads from the front and CONSUMES; throws std::invalid_argument on underflow
//
// SEMANTICS NOTE: the ORIGINAL client's StlBuffer::operator>> appears to PEEK (Game.cpp:235-236 does
//   `data >> opcode; data.eraseFront(sizeof(opcode));` — a consume + eraseFront would skip 2 payload bytes).
//   This reconstruction makes `>>` CONSUME (validated wire-correct in build/harness). To drop these headers
//   into the UNMODIFIED original client source, either (a) change `>>` here to peek and have each read do
//   `>> x; eraseFront(sizeof(x));`, or (b) patch the one site Game.cpp:236 to drop the extra eraseFront.
//   Our reconstructed packet unpack() bodies use the CONSUME convention (plain chained `>>`).
class StlBuffer {
public:
    StlBuffer() = default;
    StlBuffer(const std::uint8_t* p, std::size_t n) : m_data(p, p + n) {}

    std::size_t size() const { return m_data.size() - m_read; }
    bool empty() const { return size() == 0; }
    void eraseFront(std::size_t n) {
        if (n > size()) throw std::invalid_argument("StlBuffer::eraseFront overflow");
        m_read += n;
    }
    const std::vector<std::uint8_t>& raw() const { return m_data; }

    // ---- write (append, little-endian) ----
    template <class T> StlBuffer& writePod(const T& v) {
        const std::uint8_t* p = reinterpret_cast<const std::uint8_t*>(&v);
        m_data.insert(m_data.end(), p, p + sizeof(T));
        return *this;
    }
    StlBuffer& operator<<(std::uint8_t v)  { return writePod(v); }
    StlBuffer& operator<<(std::int8_t v)   { return writePod(v); }
    StlBuffer& operator<<(bool v)          { std::uint8_t b = v ? 1 : 0; return writePod(b); }
    StlBuffer& operator<<(std::uint16_t v) { return writePod(v); }
    StlBuffer& operator<<(std::int16_t v)  { return writePod(v); }
    StlBuffer& operator<<(std::uint32_t v) { return writePod(v); }
    StlBuffer& operator<<(std::int32_t v)  { return writePod(v); }
    StlBuffer& operator<<(std::uint64_t v) { return writePod(v); }
    StlBuffer& operator<<(std::int64_t v)  { return writePod(v); }
    StlBuffer& operator<<(float v)         { return writePod(v); }
    StlBuffer& operator<<(double v)        { return writePod(v); }
    StlBuffer& operator<<(const std::string& s) {
        m_data.insert(m_data.end(), s.begin(), s.end());
        m_data.push_back(0);
        return *this;
    }
    StlBuffer& operator<<(const char* s) { return (*this) << std::string(s); }

    // ---- read (consume front, little-endian; throws on underflow) ----
    template <class T> StlBuffer& readPod(T& out) {
        if (size() < sizeof(T)) throw std::invalid_argument("StlBuffer::read underflow");
        std::memcpy(&out, m_data.data() + m_read, sizeof(T));
        m_read += sizeof(T);
        return *this;
    }
    StlBuffer& operator>>(std::uint8_t& v)  { return readPod(v); }
    StlBuffer& operator>>(std::int8_t& v)   { return readPod(v); }
    StlBuffer& operator>>(bool& v)          { std::uint8_t b; readPod(b); v = b != 0; return *this; }
    StlBuffer& operator>>(std::uint16_t& v) { return readPod(v); }
    StlBuffer& operator>>(std::int16_t& v)  { return readPod(v); }
    StlBuffer& operator>>(std::uint32_t& v) { return readPod(v); }
    StlBuffer& operator>>(std::int32_t& v)  { return readPod(v); }
    StlBuffer& operator>>(std::uint64_t& v) { return readPod(v); }
    StlBuffer& operator>>(std::int64_t& v)  { return readPod(v); }
    StlBuffer& operator>>(float& v)         { return readPod(v); }
    StlBuffer& operator>>(double& v)        { return readPod(v); }
    StlBuffer& operator>>(std::string& s) {
        s.clear();
        for (;;) {
            if (size() < 1) throw std::invalid_argument("StlBuffer::read string underflow");
            char c = static_cast<char>(m_data[m_read++]);
            if (c == 0) break;
            s.push_back(c);
        }
        return *this;
    }

    // typed accessors referenced by the client
    std::string  getString() { std::string s; (*this) >> s; return s; }
    std::int32_t getInt32()  { std::int32_t v; (*this) >> v; return v; }
    float        getFloat()  { float v; (*this) >> v; return v; }
    std::uint32_t getGuid()  { std::uint32_t v; (*this) >> v; return v; }
    std::size_t  getSize() const { return size(); }

    bool writeFile(const std::string& path) const {
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        f.write(reinterpret_cast<const char*>(m_data.data()), static_cast<std::streamsize>(m_data.size()));
        return static_cast<bool>(f);
    }
    bool readFile(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        m_data.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        m_read = 0;
        return true;
    }

private:
    std::vector<std::uint8_t> m_data;
    std::size_t m_read = 0;
};
