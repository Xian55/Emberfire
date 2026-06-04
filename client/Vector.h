#pragma once
//
// Vector.h  (reconstructed parent-level support header)
//
// Included by the template/DB editors (GameObjTemplateEditor, ItemTemplateEditor,
// GossipEntryEditor, GossipOptionEntryEditor, LootDbEntryEditor, ...).
//
// Investigation note: across the entire client there is NO use of a global
// `Vector` type or `Vector<...>` template, and no `Vector::` qualified symbol.
// The editors that #include "..\Vector.h" only ever construct sf::Vector2f /
// sf::Vector2i (which come from SFML via stdafx.h). The header is therefore a
// thin legacy/compatibility include. To honor its name it provides small generic
// 2D/3D vector helpers in their own namespace (kept distinct from Geo2d::Vector2
// and sf::Vector2 so nothing collides), plus a std::vector convenience alias.
//
// Compilable standalone under MSVC x86, C++17.
//
#include <cmath>
#include <vector>

namespace Dmyst
{
    // Generic alias for those who spell std::vector as Vector.
    template <typename T>
    using Vector = std::vector<T>;

    struct Vector2f
    {
        float x{ 0.f };
        float y{ 0.f };

        Vector2f() = default;
        Vector2f(float ix, float iy) : x(ix), y(iy) {}

        Vector2f operator+(const Vector2f& o) const { return { x + o.x, y + o.y }; }
        Vector2f operator-(const Vector2f& o) const { return { x - o.x, y - o.y }; }
        Vector2f operator*(float s) const { return { x * s, y * s }; }

        float length() const { return std::sqrt(x * x + y * y); }
        float dist(const Vector2f& o) const { return (*this - o).length(); }
    };

    struct Vector3f
    {
        float x{ 0.f };
        float y{ 0.f };
        float z{ 0.f };

        Vector3f() = default;
        Vector3f(float ix, float iy, float iz) : x(ix), y(iy), z(iz) {}

        Vector3f operator+(const Vector3f& o) const { return { x + o.x, y + o.y, z + o.z }; }
        Vector3f operator-(const Vector3f& o) const { return { x - o.x, y - o.y, z - o.z }; }
        Vector3f operator*(float s) const { return { x * s, y * s, z * s }; }

        float length() const { return std::sqrt(x * x + y * y + z * z); }
    };
}
