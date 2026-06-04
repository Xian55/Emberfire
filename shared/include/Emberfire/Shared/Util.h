#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
// Free helpers used across the client (referenced as Util::xxx). std-only ones are defined inline here;
// the few that need SFML types (sf::Color/sf::Vector2/sf::Keyboard) are declared only — // TODO: provide in SFML build.

namespace Util {

// printf-style -> std::string
inline std::string fmtStr(const char* fmt, ...) {
    va_list a; va_start(a, fmt);
    va_list b; va_copy(b, a);
    int n = std::vsnprintf(nullptr, 0, fmt, a); va_end(a);
    std::string s(n < 0 ? 0 : n, '\0');
    if (n > 0) std::vsnprintf(&s[0], n + 1, fmt, b);
    va_end(b);
    return s;
}

inline std::string base64_encode(const std::string& in) {
    static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string o; int val = 0, bits = -6;
    for (unsigned char c : in) { val = (val << 8) + c; bits += 8;
        while (bits >= 0) { o.push_back(T[(val >> bits) & 0x3f]); bits -= 6; } }
    if (bits > -6) o.push_back(T[((val << 8) >> (bits + 8)) & 0x3f]);
    while (o.size() % 4) o.push_back('=');
    return o;
}

inline void strReplaceAll(std::string& subject, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    std::size_t p = 0;
    while ((p = subject.find(from, p)) != std::string::npos) { subject.replace(p, from.size(), to); p += to.size(); }
}

#ifndef DMYST_HAVE_UTIL_MASKHAS
#define DMYST_HAVE_UTIL_MASKHAS
template <class A, class B> inline bool maskHas(A mask, B flag) {
    return (static_cast<std::uint64_t>(mask) & static_cast<std::uint64_t>(flag)) != 0;
}
#endif

inline void trimStr(std::string& s) {
    auto a = s.find_first_not_of(" \t\r\n"); auto b = s.find_last_not_of(" \t\r\n");
    s = (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
}
inline std::string toLowerCase(const std::string& s) {
    std::string r = s; std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return r;
}
inline void splitStr(const std::string& in, std::vector<std::string>& out, char delim) {
    std::stringstream ss(in); std::string item;
    while (std::getline(ss, item, delim)) out.push_back(item);
}
inline void readLinesFromFile(const std::string& path, std::vector<std::string>& out) {
    std::ifstream f(path); std::string line; while (std::getline(f, line)) out.push_back(line);
}
inline std::string readTextFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str();
}
inline std::string formatMoneyCommas(long long amount) {
    std::string s = std::to_string(amount < 0 ? -amount : amount);
    for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert(i, ",");
    return (amount < 0 ? "-" : "") + s;
}
inline std::string toRoman(int n) {
    static const int v[] = {1000,900,500,400,100,90,50,40,10,9,5,4,1};
    static const char* r[] = {"M","CM","D","CD","C","XC","L","XL","X","IX","V","IV","I"};
    std::string s; for (int i = 0; i < 13 && n > 0; ++i) while (n >= v[i]) { s += r[i]; n -= v[i]; } return s;
}
inline std::string formatTimeBySeconds(int sec) {
    int h = sec/3600, m = (sec%3600)/60, s = sec%60; char b[32];
    if (h) std::snprintf(b,sizeof b,"%d:%02d:%02d",h,m,s); else std::snprintf(b,sizeof b,"%d:%02d",m,s);
    return b;
}

// --- RNG (TODO: match the server's RNG if determinism ever matters; client-side cosmetic uses) ---
int  irand(int lo, int hi);            // inclusive
float frand(float lo, float hi);
bool roll_chance_i(int percent);
template <class T, class... R> T randomChoice(T first, R... rest) { T arr[] = { first, static_cast<T>(rest)... }; return arr[irand(0, (int)sizeof...(rest))]; }

// --- need SFML / more context: declared only, provide in the SFML build (TODO) ---
struct GeoBox2d { int x = 0, y = 0, w = 0, h = 0; GeoBox2d() = default; GeoBox2d(int X,int Y,int W,int H):x(X),y(Y),w(W),h(H){} };
int parseIntExpression(const std::string& expr);   // TODO arithmetic expr eval
bool compareNaturalSort(const std::string& a, const std::string& b);
void getFileList(const char* dir, std::vector<std::string>& out);   // TODO platform dir listing
std::wstring toUtf16(const std::string& s);

} // namespace Util
