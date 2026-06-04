#pragma once
//
// StringHelpers.h  (reconstructed parent-level support header)
//
// All helpers live in namespace Util. Reconstructed from client call sites:
//
//   Util::fmtStr("...%d...", a)            -> std::string  (printf-style)
//   Util::strReplaceAll(str, from, to)     -> in-place replace, modifies 'str'
//   Util::trimStr(str)                     -> in-place trim, modifies 'str'
//   Util::toLowerCase(str)                 -> std::string (copy, lowered)
//   Util::splitStr(src, out, delim)        -> template; tokenizes into vector<T>
//   Util::toUtf16(str)                     -> std::wstring
//   Util::formatMoneyCommas(int64)         -> std::string ("1,234,567")
//   Util::formatTimeBySeconds(int)         -> std::string ("1h 2m 3s")
//   Util::toRoman(int)                     -> std::string
//   Util::parseIntExpression(str)          -> int (evaluates simple +-* / int expr)
//   Util::compareNaturalSort(a, b)         -> bool (natural/alphanumeric sort comparator)
//   Util::base64_encode(str)               -> std::string
//   Util::maskHas(mask, flag)              -> bool (bitmask test, template)
//   Util::simulateInputBox(label, value&)  -> blocking console/Win32 input box (stubbed)
//   Util::sfKeyToString(sf::Keyboard::Key) -> std::string
//
// Compilable standalone under MSVC x86, C++17.
//
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdint>

#include <SFML/Window/Keyboard.hpp>

// Math.h (same parent dir) provides Util::cordsInBox / brightenColor / GeoBox2d.
// Some client TUs (e.g. MapQuester.cpp) include StringHelpers.h but not Math.h yet
// reference Util::cordsInBox; chain it here so those resolve. Math.h is #pragma once.
#include "Math.h"

namespace Util
{
    // ---- printf-style formatting -------------------------------------------
    inline std::string fmtStr(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        va_list copy;
        va_copy(copy, args);
        const int needed = std::vsnprintf(nullptr, 0, fmt, copy);
        va_end(copy);

        if (needed <= 0)
        {
            va_end(args);
            return std::string();
        }

        std::string result(static_cast<size_t>(needed), '\0');
        std::vsnprintf(&result[0], static_cast<size_t>(needed) + 1, fmt, args);
        va_end(args);
        return result;
    }

    // ---- in-place replace all ----------------------------------------------
    inline void strReplaceAll(std::string& str, const std::string& from, const std::string& to)
    {
        if (from.empty())
            return;
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos)
        {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    // ---- in-place trim (leading/trailing whitespace) -----------------------
    inline void trimStr(std::string& str)
    {
        const char* ws = " \t\r\n";
        const size_t first = str.find_first_not_of(ws);
        if (first == std::string::npos)
        {
            str.clear();
            return;
        }
        const size_t last = str.find_last_not_of(ws);
        str = str.substr(first, last - first + 1);
    }

    // ---- to lower (copy) ----------------------------------------------------
    inline std::string toLowerCase(const std::string& str)
    {
        std::string out = str;
        std::transform(out.begin(), out.end(), out.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return out;
    }

    // ---- tokenizing split (templated on element type) ----------------------
    // Used as: splitStr(string, vector<string>&, ' ') and
    //          splitStr(const char*, vector<int>&, ',')
    namespace detail
    {
        inline void assignToken(const std::string& token, std::string& out) { out = token; }
        inline void assignToken(const std::string& token, int& out) { out = std::atoi(token.c_str()); }
        inline void assignToken(const std::string& token, long& out) { out = std::atol(token.c_str()); }
        inline void assignToken(const std::string& token, float& out) { out = static_cast<float>(std::atof(token.c_str())); }
        inline void assignToken(const std::string& token, double& out) { out = std::atof(token.c_str()); }
    }

    template <typename T>
    inline void splitStr(const std::string& src, std::vector<T>& out, char delim)
    {
        std::stringstream ss(src);
        std::string item;
        while (std::getline(ss, item, delim))
        {
            T value{};
            detail::assignToken(item, value);
            out.push_back(value);
        }
    }

    // ---- utf8/ansi -> utf16 -------------------------------------------------
    inline std::wstring toUtf16(const std::string& str)
    {
        return std::wstring(str.begin(), str.end());
    }

    // ---- "1234567" -> "1,234,567" ------------------------------------------
    inline std::string formatMoneyCommas(long long amount)
    {
        const bool neg = amount < 0;
        unsigned long long v = neg ? static_cast<unsigned long long>(-amount)
                                   : static_cast<unsigned long long>(amount);
        std::string digits = std::to_string(v);

        std::string out;
        int count = 0;
        for (auto it = digits.rbegin(); it != digits.rend(); ++it)
        {
            if (count != 0 && count % 3 == 0)
                out.push_back(',');
            out.push_back(*it);
            ++count;
        }
        std::reverse(out.begin(), out.end());
        if (neg)
            out.insert(out.begin(), '-');
        return out;
    }

    // ---- seconds -> "Xd Yh Zm Ws" (compact) --------------------------------
    inline std::string formatTimeBySeconds(int totalSeconds)
    {
        if (totalSeconds < 0)
            totalSeconds = 0;

        const int days = totalSeconds / 86400; totalSeconds %= 86400;
        const int hours = totalSeconds / 3600; totalSeconds %= 3600;
        const int minutes = totalSeconds / 60;
        const int seconds = totalSeconds % 60;

        std::string out;
        auto append = [&out](int value, const char* suffix)
        {
            if (value <= 0)
                return;
            if (!out.empty())
                out.push_back(' ');
            out += std::to_string(value);
            out += suffix;
        };

        append(days, "d");
        append(hours, "h");
        append(minutes, "m");
        if (out.empty() || seconds > 0)
        {
            if (!out.empty())
                out.push_back(' ');
            out += std::to_string(seconds);
            out += "s";
        }
        return out;
    }

    // ---- integer -> roman numerals -----------------------------------------
    inline std::string toRoman(int number)
    {
        if (number <= 0)
            return std::string();

        static const int values[] = { 1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1 };
        static const char* numerals[] = { "M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I" };

        std::string out;
        for (int i = 0; i < 13; ++i)
        {
            while (number >= values[i])
            {
                out += numerals[i];
                number -= values[i];
            }
        }
        return out;
    }

    // ---- evaluate a simple integer expression (+ - * /), left-to-right ------
    inline int parseIntExpression(const std::string& expr)
    {
        // Tokenizes numbers and the operators + - * / and evaluates them
        // left-to-right (no operator precedence). Good enough for the client's
        // tooltip macros (e.g. "value*clvl+5").
        std::vector<long long> numbers;
        std::vector<char> ops;

        size_t i = 0;
        const size_t n = expr.size();
        auto skipWs = [&]() { while (i < n && std::isspace(static_cast<unsigned char>(expr[i]))) ++i; };

        skipWs();
        bool expectNumber = true;
        while (i < n)
        {
            skipWs();
            if (i >= n)
                break;

            if (expectNumber)
            {
                bool neg = false;
                if (expr[i] == '+' || expr[i] == '-')
                {
                    neg = (expr[i] == '-');
                    ++i;
                    skipWs();
                }
                long long value = 0;
                bool any = false;
                while (i < n && std::isdigit(static_cast<unsigned char>(expr[i])))
                {
                    value = value * 10 + (expr[i] - '0');
                    ++i;
                    any = true;
                }
                if (!any)
                    break;
                numbers.push_back(neg ? -value : value);
                expectNumber = false;
            }
            else
            {
                const char c = expr[i];
                if (c == '+' || c == '-' || c == '*' || c == '/')
                {
                    ops.push_back(c);
                    ++i;
                    expectNumber = true;
                }
                else
                {
                    ++i; // skip unknown char
                }
            }
        }

        if (numbers.empty())
            return 0;

        long long result = numbers[0];
        for (size_t k = 0; k < ops.size() && (k + 1) < numbers.size(); ++k)
        {
            const long long rhs = numbers[k + 1];
            switch (ops[k])
            {
            case '+': result += rhs; break;
            case '-': result -= rhs; break;
            case '*': result *= rhs; break;
            case '/': result = (rhs != 0) ? result / rhs : 0; break;
            default: break;
            }
        }
        return static_cast<int>(result);
    }

    // ---- natural / alphanumeric sort comparator ----------------------------
    // Returns true if 'a' should be ordered before 'b'.
    inline bool compareNaturalSort(const std::string& a, const std::string& b)
    {
        size_t i = 0, j = 0;
        const size_t na = a.size(), nb = b.size();
        while (i < na && j < nb)
        {
            const unsigned char ca = static_cast<unsigned char>(a[i]);
            const unsigned char cb = static_cast<unsigned char>(b[j]);

            if (std::isdigit(ca) && std::isdigit(cb))
            {
                // Compare full numeric runs by value.
                size_t si = i, sj = j;
                while (i < na && std::isdigit(static_cast<unsigned char>(a[i]))) ++i;
                while (j < nb && std::isdigit(static_cast<unsigned char>(b[j]))) ++j;

                std::string numA = a.substr(si, i - si);
                std::string numB = b.substr(sj, j - sj);

                // Strip leading zeros for length comparison.
                size_t za = numA.find_first_not_of('0');
                size_t zb = numB.find_first_not_of('0');
                numA = (za == std::string::npos) ? "0" : numA.substr(za);
                numB = (zb == std::string::npos) ? "0" : numB.substr(zb);

                if (numA.size() != numB.size())
                    return numA.size() < numB.size();
                if (numA != numB)
                    return numA < numB;
            }
            else
            {
                const unsigned char la = static_cast<unsigned char>(std::tolower(ca));
                const unsigned char lb = static_cast<unsigned char>(std::tolower(cb));
                if (la != lb)
                    return la < lb;
                ++i;
                ++j;
            }
        }
        return (na - i) < (nb - j);
    }

    // ---- base64 encode ------------------------------------------------------
    inline std::string base64_encode(const std::string& in)
    {
        static const char* tbl =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string out;
        int val = 0;
        int bits = -6;
        for (unsigned char c : in)
        {
            val = (val << 8) + c;
            bits += 8;
            while (bits >= 0)
            {
                out.push_back(tbl[(val >> bits) & 0x3F]);
                bits -= 6;
            }
        }
        if (bits > -6)
            out.push_back(tbl[((val << 8) >> (bits + 8)) & 0x3F]);
        while (out.size() % 4)
            out.push_back('=');
        return out;
    }

    // ---- bitmask test -------------------------------------------------------
    // (Guarded so it doesn't clash with the identical copy in SpellDefines.h
    //  when both headers are pulled into one TU.)
#ifndef DMYST_HAVE_UTIL_MASKHAS
#define DMYST_HAVE_UTIL_MASKHAS
    template <typename M, typename F>
    inline bool maskHas(M mask, F flag)
    {
        return (static_cast<unsigned long long>(mask) &
                static_cast<unsigned long long>(flag)) != 0ull;
    }
#endif

    // ---- blocking input box (stub) -----------------------------------------
    // The original popped a native input box and wrote the entered text back
    // into 'value'. Reconstructed as a no-op that leaves 'value' unchanged so
    // editor code compiles and runs without a modal dependency.
    inline void simulateInputBox(const std::string& /*label*/, std::string& /*value*/)
    {
        // No-op: original opened a modal text-entry box.
    }

    // sfKeyToString + brightenColor are provided by the client's SfExtensions.h (namespace Util).
    // Disabled here to avoid duplicate-definition.
#if 0
    inline std::string sfKeyToString(sf::Keyboard::Key key)
    {
        using K = sf::Keyboard;
        if (key >= K::A && key <= K::Z)
            return std::string(1, static_cast<char>('A' + (key - K::A)));
        if (key >= K::Num0 && key <= K::Num9)
            return std::string(1, static_cast<char>('0' + (key - K::Num0)));
        if (key >= K::Numpad0 && key <= K::Numpad9)
            return "Num" + std::to_string(key - K::Numpad0);
        if (key >= K::F1 && key <= K::F15)
            return "F" + std::to_string(key - K::F1 + 1);

        switch (key)
        {
        case K::Escape:    return "Escape";
        case K::LControl:  return "LCtrl";
        case K::LShift:    return "LShift";
        case K::LAlt:      return "LAlt";
        case K::RControl:  return "RCtrl";
        case K::RShift:    return "RShift";
        case K::RAlt:      return "RAlt";
        case K::Space:     return "Space";
        case K::Return:    return "Enter";
        case K::BackSpace: return "Backspace";
        case K::Tab:       return "Tab";
        case K::Left:      return "Left";
        case K::Right:     return "Right";
        case K::Up:        return "Up";
        case K::Down:      return "Down";
        case K::Tilde:     return "~";
        case K::Comma:     return ",";
        case K::Period:    return ".";
        default:           return "Key" + std::to_string(static_cast<int>(key));
        }
    }
#endif
}
