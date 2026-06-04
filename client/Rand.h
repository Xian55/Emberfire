#pragma once
//
// Rand.h  (reconstructed parent-level support header)
//
// RNG helpers in namespace Util. Reconstructed from client call sites:
//
//   Util::irand(min, max)        -> int in [min, max]   (inclusive)
//   Util::frand(min, max)        -> float in [min, max]
//   Util::roll_chance_i(percent) -> bool; true with 'percent'% probability (0..100)
//   Util::randomChoice(a, b, ...)-> picks one of the arguments uniformly, returns by value
//                                   (called with string / const char* / bool / enum args)
//
// Compilable standalone under MSVC x86, C++17.
//
#include <random>
#include <initializer_list>
#include <type_traits>

namespace Util
{
    namespace detail
    {
        inline std::mt19937& rng()
        {
            static std::mt19937 engine(std::random_device{}());
            return engine;
        }
    }

    // Inclusive integer in [min, max].
    inline int irand(int minValue, int maxValue)
    {
        if (minValue > maxValue)
            std::swap(minValue, maxValue);
        std::uniform_int_distribution<int> dist(minValue, maxValue);
        return dist(detail::rng());
    }

    // Float in [min, max].
    inline float frand(float minValue, float maxValue)
    {
        if (minValue > maxValue)
            std::swap(minValue, maxValue);
        std::uniform_real_distribution<float> dist(minValue, maxValue);
        return dist(detail::rng());
    }

    // Returns true with 'percent'% probability (percent in 0..100).
    inline bool roll_chance_i(int percent)
    {
        if (percent <= 0)
            return false;
        if (percent >= 100)
            return true;
        return irand(1, 100) <= percent;
    }

    // Pick one of the supplied arguments uniformly at random.
    // All arguments must share a common type; result is returned by value.
    template <typename First, typename... Rest>
    inline std::common_type_t<First, Rest...> randomChoice(First&& first, Rest&&... rest)
    {
        using Result = std::common_type_t<First, Rest...>;
        const Result options[] = {
            static_cast<Result>(std::forward<First>(first)),
            static_cast<Result>(std::forward<Rest>(rest))...
        };
        const int count = 1 + static_cast<int>(sizeof...(Rest));
        return options[irand(0, count - 1)];
    }
}
