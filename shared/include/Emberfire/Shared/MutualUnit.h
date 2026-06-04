#pragma once
//
// MutualUnit — reconstructed shared base for units (players + npcs).
// Client: ClientUnit : public ClientObject, public MutualUnit.
//
// NOTE: MutualUnit is a SEPARATE base from MutualObject in the client's multiple
// inheritance (ClientUnit derives from both ClientObject — which has MutualObject —
// and MutualUnit). To read unit stats/health it needs access to the object variable
// store; ClientUnit bridges that. To keep this header self-contained and compilable
// without pulling MutualObject in via the diamond, MutualUnit exposes virtual
// accessors that the concrete client class wires to its variable store.
//
// Most numeric unit state is derived from ObjDefines::Variable entries:
//   Health(0x01), Level(0x02), MaxHealth(0x03), Mana(0x04), MaxMana, plus the
//   contiguous stat block StatsStart(0x0e)+UnitDefines::Stat index (see PROTOCOL.md).
//
#include <vector>
#include <map>

#include "Geo2d.h"
#include "UnitDefines.h"   // UnitDefines::Stat / Faction (written by enum agent)
#include "GameDefines.h"   // ObjDefines::Variable
#include "PlayerDefines.h" // PlayerDefines::Classes (player stat/equip helpers)
#include "ItemDefines.h"   // ItemDefines::EquipType / WeaponType / ArmorType / WeaponMaterial

class MutualUnit
{
public:
    MutualUnit() = default;
    // ClientUnit constructs MutualUnit with a reference to its world position
    // (the spline drives that position). Stored for spline interpolation.
    explicit MutualUnit(Geo2d::Vector2& worldPosRef) : m_worldPosRef(&worldPosRef) {}
    virtual ~MutualUnit() = default;

    // ---- movement speed (MoveSpeedPct object variable) ----
    // getSpeed() is used as an animation-rate multiplier (1.0 == base speed).
    float getSpeed() const { return m_speed; }
    void setSpeed(const float s) { m_speed = s; }

    // ---- core unit state ----
    // NOTE: health / level / generic stats (isAlive, getHealth, getLevel, getStat,
    // getHealthPct) live on MutualObject — the concrete ClientUnit derives from both
    // MutualObject (via ClientObject) and MutualUnit, so duplicating them here would
    // make calls on a ClientUnit ambiguous. Mana-specific helpers stay here because
    // they are unit-only and read through the virtual getUnitVariable() hook.
    int getMana() const { return getUnitVariable(ObjDefines::Variable::Mana); }
    // There is no dedicated MaxMana object var; max mana is the Mana stat (var 0x0f).
    // TODO: confirm server uses the Mana stat as the mana cap.
    int getMaxMana() const
    {
        const int base = static_cast<int>(ObjDefines::Variable::StatsStart);
        return getUnitVariable(static_cast<ObjDefines::Variable>(
            base + static_cast<int>(UnitDefines::Stat::Mana)));
    }

    float getManaPct() const
    {
        const int mx = getMaxMana();
        return mx > 0 ? static_cast<float>(getMana()) / static_cast<float>(mx) : 0.f;
    }

    // ---- player stat / equip helpers ----
    // Called on a ClientPlayer (which derives MutualUnit). Reconstructed stubs:
    // the real values come from the server's stat sync + class equip rules.
    // TODO: implement to match server PlayerFunctions / equip tables.
    int getBaseStat(const UnitDefines::Stat stat) const
    {
        // Base (un-equipped) stat value. Stubbed to the synced stat var.
        const int base = static_cast<int>(ObjDefines::Variable::StatsStart);
        return getUnitVariable(static_cast<ObjDefines::Variable>(base + static_cast<int>(stat)));
    }
    int getBaseStatBonus(const UnitDefines::Stat /*stat*/) const { return 0; }

    void applyStatModifierLogic(const PlayerDefines::Classes /*cls*/,
                                const UnitDefines::Stat /*stat*/,
                                int& /*value*/,
                                std::map<UnitDefines::Stat, int>& /*allStats*/) const
    {
        // TODO: real impl must match server stat-modifier formula.
    }

    // Class can equip this item category.
    // GROUND TRUTH: the original client tooltip gate (ItemIcon "Can never use this type" → red
    // material+type text) calls these via myself->canEquipItem(...). The proficiency rule is the
    // inlined FUN_0044f0d0 (@0x0044f0d0): wireId-0x1c selects the weapon-skill row 0..7, class read
    // from *(*(this+0xd0)+0x1ac) (1-based). Recovered class×weaponType table is in
    // PlayerFunctions::canEquipItem; delegate to it so the member overloads (what ItemIcon calls)
    // enforce the same rule as Equipment::canUseSkill. (Were stubbed `return true` → r1190 showed
    // every material/type white, i.e. Mage could "use" an Aspen Bow.)
    // The ArmorType/WeaponMaterial overloads take an optional out-map of stat
    // requirements that weren't met (empty == requirements satisfied).
    bool canEquipItem(const PlayerDefines::Classes cls, const ItemDefines::EquipType t) const
    {
        return PlayerFunctions::canEquipItem(cls, t);
    }
    bool canEquipItem(const PlayerDefines::Classes cls, const ItemDefines::WeaponType t) const
    {
        return PlayerFunctions::canEquipItem(cls, t);
    }
    bool canEquipItem(const PlayerDefines::Classes cls, const ItemDefines::ArmorType t, const ItemDefines::EquipType e,
                      std::map<UnitDefines::Stat, int>* failedStatReq = nullptr) const
    {
        // Armor is gated by base-stat requirements (server FUN_004747a0). Returns false if any required base
        // stat is unmet (drives the red inventory-icon outline in computeHasErrorIcon); fills failedStatReq
        // with the unmet stats (drives the tooltip "Requires N base X" red lines). Tooltip ignores the return.
        bool ok = true;
        for (const auto& r : PlayerFunctions::getRequiredStats(cls, t, e))
        {
            if (getBaseStat(r.first) < r.second)
            {
                ok = false;
                if (failedStatReq != nullptr)
                    (*failedStatReq)[r.first] = r.second;
            }
        }
        return ok;
    }
    bool canEquipItem(const ItemDefines::WeaponType /*t*/, const ItemDefines::WeaponMaterial /*m*/,
                      std::map<UnitDefines::Stat, int>* /*failedStatReq*/ = nullptr) const { return true; }

    // Cost (in exp) to commit the given pending stat + spell investments.
    // Faithful port of client FUN_005550c0: for each pending stat, RAMP the per-point cost
    // (PlayerFunctions::computeStatUpgradeCost = FUN_005533f0) from the already-invested count
    // over `count` points. The invested count is the unit var at (wireSpendId + 0x3e) — the
    // server-synced invest block (FUN_005550c0 reads varVector[key+0x3e] as the ramp base). The
    // cost arg uses the wire-id space FUN_005533f0 switches on. 64-bit accumulate, clamp INT_MAX.
    // (Was a stub that summed point COUNTS, so pending cost ignored the curve.)
    int computeLevelupCost(const std::map<UnitDefines::Stat, int>& statInvestments,
                           const std::map<int, int>& spellInvestments) const
    {
        // wire spend-id == GP_Client_LevelUp::spendId (kept inline to avoid a packet-header include).
        auto wireId = [](UnitDefines::Stat s) -> int {
            using S = UnitDefines::Stat;
            if (s == S::Health) return 1;
            if (s == S::Mana)   return 2;
            return static_cast<int>(s);   // enum/var index == wire spend-id (Health/Mana swapped above)
        };

        long long total = 0;
        for (const auto& p : statInvestments)
        {
            if (p.second <= 0)
                continue;
            const int wid  = wireId(p.first);
            const int base = getUnitVariable(static_cast<ObjDefines::Variable>(wid + 0x3e)); // already invested
            for (int j = 0; j < p.second; ++j)
            {
                const int c = PlayerFunctions::computeStatUpgradeCost(p.first, base + j);
                if (c < 0)   // non-investable should never be in the map; guard
                    break;
                total += c;
                if (total > 0x7fffffff)
                    return 0x7fffffff;
            }
        }
        for (const auto& p : spellInvestments)
        {
            if (p.second <= 0)
                continue;
            for (int j = 0; j < p.second; ++j)            // spell ramp from 0 (op41/spell base deferred)
                total += PlayerFunctions::computeSpellUpgradeCost(j);
            if (total > 0x7fffffff)
                return 0x7fffffff;
        }
        return static_cast<int>(total);
    }

    // ---- orientation ----
    float getOrientation() const { return m_orientation; }
    void setOrientation(const float o) { m_orientation = o; }

    bool isInCombat() const { return m_inCombat; }

    // ---- faction / friendliness ----
    virtual UnitDefines::Faction faction() const { return UnitDefines::Faction::Neutral; }

    // The server resolves the per-viewer reaction and sends it AS the unit's faction value.
    // Only Friendly (green NPC) + PlayerDefault (blue self/other players) are non-attackable;
    // Neutral (yellow) and Hostile (red) are both attackable — Tab-targetable + sword cursor.
    // (Was `!= Hostile`, which wrongly made Neutral friendly -> Tab skipped neutral mobs.)
    bool seesAsFriendly(const MutualUnit& other) const
    {
        return other.faction() == UnitDefines::Faction::Friendly
            || other.faction() == UnitDefines::Faction::PlayerDefault;
    }

    // Only Hostile auto-aggros / counts as an active enemy. Neutral is the in-between: attackable
    // (!seesAsFriendly) but NOT seesAsHostile.
    bool seesAsHostile(const MutualUnit& other) const
    {
        return other.faction() == UnitDefines::Faction::Hostile;
    }

    // ---- spline (movement path) ----
    bool hasSpline() const { return !m_spline.empty(); }
    const std::vector<Geo2d::Vector2>& spline() const { return m_spline; }
    bool isSlidingSpline() const { return m_slidingSpline; }

    void setSpline(const std::vector<Geo2d::Vector2>& s, const bool sliding = false)
    {
        m_spline = s;
        m_slidingSpline = sliding;
    }
    void clearSpline()
    {
        m_spline.clear();
        m_slidingSpline = false;
    }

    // ---- spline advance (two overloads, matching ClientUnit::update call sites) ----
    //
    // pumpSpline(serverSpline, serverLocation, delta): step the rendered world
    //   position along the active path. The op90 handler (processPacket_Server_UnitSpline)
    //   is authoritative: it sets the path + start position and reconciles drift, and
    //   Geo2d::extrude() clamps at the final node — so the unit ALWAYS starts and ends at
    //   the correct position; this only smooths the transit between server updates.
    //
    // pumpSpline() (no-arg): true once the path is exhausted (unit idle). Used as
    //   `!pumpSpline() && hasSpline()` → true while still travelling → Run animation.
    //
    // NOTE: the server's per-frame step (FUN_004b56c0) consumes a budget = speed*dt, but
    // the engine's BASE move-speed constant is NOT recoverable from the stripped server
    // dump. BASE_MOVE_SPEED below is a client-side VISUAL estimate (world-units/sec at
    // MoveSpeedPct==100); it is safe to tune because it affects transit timing only, never
    // the start/end position (server-authoritative + extrude clamp).
    bool pumpSpline()
    {
        return m_spline.empty();
    }
    void pumpSpline(const std::vector<Geo2d::Vector2>& /*serverSpline*/,
                    const Geo2d::Vector2& /*serverLocation*/,
                    const float delta)
    {
        if (m_worldPosRef == nullptr || m_spline.empty() || delta <= 0.f)
            return;

        // m_speed is a 1.0-based move multiplier (MoveSpeedPct/100; default 1.0).
        static constexpr float BASE_MOVE_SPEED = 5.0f; // units/sec @ 100% — MEASURED vs steam r1189 (op15 stream: 5.000 u/s median). Was 6.0 guess -> client outran server -> snap-back + NPC melee desync.
        const float mult = m_speed > 0.f ? m_speed : 1.f;
        float budget = BASE_MOVE_SPEED * mult * delta;

        // Walk node-to-node, spending the per-step distance budget (mirrors server
        // FUN_004b56c0: advance toward each waypoint, pop on arrival, until spent).
        while (budget > 0.f && !m_spline.empty())
        {
            const Geo2d::Vector2 node = m_spline.front();
            const float distToNode = m_worldPosRef->getDist(node);

            // Face the segment being travelled; server derives orient via atan2 and wraps
            // a negative result into [0, 2π) — match that so sprite facing agrees.
            if (distToNode > 0.0001f)
            {
                float a = std::atan2(node.y - m_worldPosRef->y, node.x - m_worldPosRef->x);
                if (a < 0.f) a += 6.28318530718f;
                m_orientation = a;
            }

            if (budget >= distToNode)
            {
                *m_worldPosRef = node;                 // reach node, keep remaining budget
                budget -= distToNode;
                m_spline.erase(m_spline.begin());
            }
            else
            {
                *m_worldPosRef = Geo2d::extrude(m_worldPosRef->x, m_worldPosRef->y,
                                                node.x, node.y, budget);
                budget = 0.f;
            }
        }
    }

protected:
    void setInCombat(const bool b) { m_inCombat = b; }

    // The concrete client class (which also IS-A MutualObject) routes this to its
    // object variable store. Default returns 0 so a bare MutualUnit still compiles.
    // ClientUnit overrides to forward to MutualObject::getVariable.
    virtual int getUnitVariable(const ObjDefines::Variable /*var*/) const { return 0; }

private:
    float m_orientation{0.f};
    float m_speed{1.f};
    bool m_inCombat{false};
    bool m_slidingSpline{false};

    Geo2d::Vector2* m_worldPosRef{nullptr};
    std::vector<Geo2d::Vector2> m_spline;
};
