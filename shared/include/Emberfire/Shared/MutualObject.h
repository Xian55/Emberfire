#pragma once
//
// MutualObject — reconstructed shared base for all world objects (client+server).
// Harvested from client usage; see build/recon/SHARED_INTERFACE.md + PROTOCOL.md.
//
// Client derivation chain: ClientObject : ... , public MutualObject
//   ClientPlayer/ClientNpc/ClientGameObj each call initType(...) in their ctor.
//
// The object's synced state lives in an int variable store keyed by
// ObjDefines::Variable (raw int id over the wire, Server_ObjectVariable packet).
//
#include <map>
#include <string>

#include "GameDefines.h"   // (written by enum agent) provides ObjDefines::Variable
#include "UnitDefines.h"   // UnitDefines::Stat (for getStat)

class MutualObject
{
public:
    enum Type
    {
        Player,
        Npc,
        GameObject,
    };

    explicit MutualObject(const int guid) : m_guid(guid) {}
    virtual ~MutualObject() = default;

    int getGuid() const { return m_guid; }

    Type getType() const { return m_type; }

    // ---- variable store (keyed by ObjDefines::Variable, stored as raw int id) ----
    // Returns 0 for an unset variable (matches client expectations, e.g. flag masks).
    int getVariable(const ObjDefines::Variable var) const
    {
        auto itr = m_variables.find(static_cast<int>(var));
        return itr == m_variables.end() ? 0 : itr->second;
    }

    void setVariable(const ObjDefines::Variable var, const int value)
    {
        m_variables[static_cast<int>(var)] = value;
        notifyVariableChange(var, value);
    }

    // Overridden by subclasses (ClientObject/ClientUnit/ClientNpc/...) to react to
    // a changed variable. Base does nothing.
    virtual void notifyVariableChange(const ObjDefines::Variable /*var*/, const int /*value*/) {}

    // ---- name / subname ----
    const std::string& getName() const { return m_name; }
    const std::string& getSubName() const { return m_subName; }

    void setName(const std::string& name)
    {
        m_name = name;
        notifyNameChanged();
    }

    void setSubName(const std::string& subName)
    {
        m_subName = subName;
        notifySubnameChanged();
    }

    // Entry/template id (npc_template / gameobject_template / item entry).
    // Base objects have no template entry; subclasses override.
    virtual int getEntry() const { return 0; }

    // ---- unit-ish state read off the object variable store ----
    // The client calls these on a bare ClientObject& (overheads/nameplates), so
    // they live here on MutualObject (not only MutualUnit). They read the same
    // ObjDefines::Variable slots the server syncs. TODO confirm semantics.
    int getHealth() const { return getVariable(ObjDefines::Variable::Health); }
    int getMaxHealth() const { return getVariable(ObjDefines::Variable::MaxHealth); }
    int getLevel() const { return getVariable(ObjDefines::Variable::Level); }
    bool isAlive() const { return getHealth() > 0; }

    float getHealthPct() const
    {
        const int mx = getMaxHealth();
        return mx > 0 ? static_cast<float>(getHealth()) / static_cast<float>(mx) : 0.f;
    }

    // Stat block: varId = StatsStart + UnitDefines::Stat index.
    int getStat(const UnitDefines::Stat stat) const
    {
        const int base = static_cast<int>(ObjDefines::Variable::StatsStart);
        return getVariable(static_cast<ObjDefines::Variable>(base + static_cast<int>(stat)));
    }

protected:
    // Called once by each concrete subclass ctor to stamp its object type.
    void initType(const Type t) { m_type = t; }

    // Subclasses (ClientUnit/ClientGameObj) override these (final) to refresh UI.
    virtual void notifyNameChanged() {}
    virtual void notifySubnameChanged() {}

private:
    int m_guid{0};
    Type m_type{Type::Npc};

    std::string m_name;
    std::string m_subName;

    // ObjDefines::Variable (as int) -> value. Sparse.
    std::map<int, int> m_variables;
};
