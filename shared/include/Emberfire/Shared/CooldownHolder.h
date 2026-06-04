#pragma once
//
// CooldownHolder — reconstructed shared cooldown bookkeeping.
// Client: World : ... , public CooldownHolder.
//
// Tracks per-id cooldown windows as (start, end) millisecond timestamps.
// Time is __time64_t (MSVC) milliseconds-since-epoch as produced by
// sApplication->timeNowMs().
//
#include <map>
#include <utility>

#ifndef _MSC_VER
#include <cstdint>
typedef std::int64_t __time64_t;   // portability shim (real client is MSVC)
#endif

class CooldownHolder
{
public:
    // (start, end) timestamps in ms. end<=start (or absent) == not on cooldown.
    using CooldownPair = std::pair<__time64_t, __time64_t>;

    CooldownHolder() = default;
    virtual ~CooldownHolder() = default;

    // Advance/expire cooldowns given the current time. Drops any whose end has
    // passed so getCooldown() reports them as ready again.
    void updateCooldowns(const __time64_t now)
    {
        for (auto itr = m_cooldowns.begin(); itr != m_cooldowns.end(); )
        {
            if (itr->second.second <= now)
                itr = m_cooldowns.erase(itr);
            else
                ++itr;
        }
    }

    // Returns (start, end) for id, or {0,0} if not on cooldown.
    CooldownPair getCooldown(const int id) const
    {
        auto itr = m_cooldowns.find(id);
        return itr == m_cooldowns.end() ? CooldownPair{0, 0} : itr->second;
    }

    // Registers a cooldown already in progress on the server (Server_Cooldown).
    // `now` lets the server clamp an already-elapsed cooldown if needed.
    // TODO: confirm server semantics — whether `now` clamps start or is ignored.
    void registerExistingCooldown(const int id, const __time64_t start,
                                  const __time64_t end, const __time64_t /*now*/)
    {
        m_cooldowns[id] = CooldownPair{start, end};
    }

    bool isOnCooldown(const int id, const __time64_t now) const
    {
        auto itr = m_cooldowns.find(id);
        return itr != m_cooldowns.end() && itr->second.second > now;
    }

private:
    std::map<int, CooldownPair> m_cooldowns;
};
