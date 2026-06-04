#pragma once
//
// Geo2d.h  (reconstructed CANONICAL parent-level geometry header)
//
// This is the authoritative Geo2d used by both the client and the Shared structs.
// A minimal placeholder also lives at build/src/Shared/Geo2d.h; this header is kept
// API-compatible with it (same Geo2d::Vector2 surface + the cell helpers).
//
// Reconstructed from client call sites:
//   Vector2{ x, y }, copy ctor, operator + - == != , scalar * (used by velocity math),
//   getDist(const Vector2&) / getDist({x,y})
//   Geo2d::distance2d (float...)        -> float
//   Geo2d::distance2di(int/float...)    -> float
//   Geo2d::extrude(fromX, fromY, toX, toY, amount) -> Vector2
//   Geo2d::computeCellId(x, y, mapWidth)
//   Geo2d::computeCellPos(cellId, x&, y&, mapWidth)
//
// Compilable standalone under MSVC x86, C++17.
//
#include <cmath>

namespace Geo2d
{
    struct Vector2
    {
        float x{ 0.f };
        float y{ 0.f };

        Vector2() = default;
        Vector2(float ix, float iy) : x(ix), y(iy) {}

        // Euclidean distance to another point.
        float getDist(const Vector2& o) const
        {
            const float dx = x - o.x;
            const float dy = y - o.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        // Magnitude / length helpers.
        float length() const { return std::sqrt(x * x + y * y); }
        float lengthSquared() const { return x * x + y * y; }

        Vector2 operator-(const Vector2& o) const { return Vector2(x - o.x, y - o.y); }
        Vector2 operator+(const Vector2& o) const { return Vector2(x + o.x, y + o.y); }
        Vector2 operator*(float s) const { return Vector2(x * s, y * s); }
        Vector2 operator/(float s) const { return Vector2(x / s, y / s); }

        Vector2& operator+=(const Vector2& o) { x += o.x; y += o.y; return *this; }
        Vector2& operator-=(const Vector2& o) { x -= o.x; y -= o.y; return *this; }
        Vector2& operator*=(float s) { x *= s; y *= s; return *this; }

        bool operator==(const Vector2& o) const { return x == o.x && y == o.y; }
        bool operator!=(const Vector2& o) const { return !(*this == o); }

        // ---- in-place mutating helpers (used by the client's render/camera math) ----
        void add(const Vector2& o) { x += o.x; y += o.y; }
        void subtract(const Vector2& o) { x -= o.x; y -= o.y; }
        void floorSelf() { x = std::floor(x); y = std::floor(y); }
        void clear() { x = 0.f; y = 0.f; }
        bool isNull() const { return x == 0.f && y == 0.f; }
    };

    inline Vector2 operator*(float s, const Vector2& v) { return Vector2(v.x * s, v.y * s); }

    // ---- free distance helpers (used directly with raw coordinates) ----

    inline float distance2d(float x1, float y1, float x2, float y2)
    {
        const float dx = x1 - x2;
        const float dy = y1 - y2;
        return std::sqrt(dx * dx + dy * dy);
    }

    inline float distance2di(int x1, int y1, int x2, int y2)
    {
        const float dx = static_cast<float>(x1 - x2);
        const float dy = static_cast<float>(y1 - y2);
        return std::sqrt(dx * dx + dy * dy);
    }

    // Move from (fromX,fromY) toward (toX,toY) by 'amount' units; never overshoots.
    inline Vector2 extrude(float fromX, float fromY, float toX, float toY, float amount)
    {
        const float dx = toX - fromX;
        const float dy = toY - fromY;
        const float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= 0.0f || amount >= dist)
            return Vector2(toX, toY);

        const float t = amount / dist;
        return Vector2(fromX + dx * t, fromY + dy * t);
    }

    // ---- map cell <-> coordinate helpers (row-major: index = y * mapWidth + x) ----

    inline int computeCellId(int x, int y, int mapWidth)
    {
        return y * mapWidth + x;
    }

    inline void computeCellPos(int cellId, int& x, int& y, int mapWidth)
    {
        x = mapWidth ? (cellId % mapWidth) : 0;
        y = mapWidth ? (cellId / mapWidth) : 0;
    }
}
