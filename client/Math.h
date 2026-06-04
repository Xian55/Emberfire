#pragma once
//
// Math.h  (reconstructed parent-level support header)
//
// Math / geometry-box helpers in namespace Util. Reconstructed from client call sites:
//
//   Util::GeoBox2d                       -> { int x, y, w, h; } axis-aligned box,
//                                           ctor GeoBox2d(x, y, w, h)
//   Util::cordsInBox(point, origin, w, h)-> bool; point/origin are sf::Vector2i
//   Util::brightenColor(color, factor)   -> sf::Color brightened by 'factor'
//
//   plus small generic numeric helpers (clamp / lerp) commonly bundled here.
//
// GeoBox2d members are accessed as .x .y .w .h (ints) by TextLines.
//
// Compilable standalone under MSVC x86, C++17.
//
#include <algorithm>
#include <cmath>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>

// MSVC only defines M_PI/M_PI_2 when _USE_MATH_DEFINES is set before <cmath>.
// Several client .cpp files use M_PI / M_PI_2 and include this header, so provide
// them unconditionally here as a fallback.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

namespace Util
{
    // Axis-aligned 2D box. Members deliberately named x/y/w/h (used by TextLines).
    struct GeoBox2d
    {
        int x{ 0 };
        int y{ 0 };
        int w{ 0 };
        int h{ 0 };

        GeoBox2d() = default;
        GeoBox2d(int ix, int iy, int iw, int ih) : x(ix), y(iy), w(iw), h(ih) {}

        bool contains(int px, int py) const
        {
            return px >= x && px < x + w && py >= y && py < y + h;
        }
    };

    // Is 'point' within the rectangle anchored at 'origin' of size (width,height)?
    inline bool cordsInBox(const sf::Vector2i& point, const sf::Vector2i& origin, int width, int height)
    {
        return point.x >= origin.x && point.x <= origin.x + width &&
               point.y >= origin.y && point.y <= origin.y + height;
    }

    // Brighten an sf::Color by lerping each channel toward white by 'factor'
    // (factor is a fraction, e.g. 0.2f = 20% toward white). Clamped to 0..1.
    // brightenColor is provided by the client's SfExtensions.h (namespace Util) — disabled to avoid dup.
#if 0
    inline sf::Color brightenColor(const sf::Color& color, float factor)
    {
        if (factor < 0.f) factor = 0.f;
        if (factor > 1.f) factor = 1.f;
        auto bump = [factor](sf::Uint8 channel) -> sf::Uint8
        {
            const float v = channel + (255.f - channel) * factor;
            return static_cast<sf::Uint8>(v + 0.5f);
        };
        return sf::Color(bump(color.r), bump(color.g), bump(color.b), color.a);
    }
#endif

    // ---- generic numeric helpers -------------------------------------------
    template <typename T>
    inline T clampValue(T value, T lo, T hi)
    {
        return value < lo ? lo : (value > hi ? hi : value);
    }

    template <typename T>
    inline T lerp(T a, T b, float t)
    {
        return static_cast<T>(a + (b - a) * t);
    }
}
