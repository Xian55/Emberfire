#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "UnitDefines.h" // ItemAffix exposes its stats as UnitDefines::Stat (required by statOrder())

// One affix_template row (the prefix/suffix that augments a generated item). The client only calls
// statOrder() (the ordered list of stats this affix grants, used to render them first in tooltips);
// the full row data is loaded by sItemDefiner from affix_template (stat_type1..5 / stat_value1..5).
class ItemAffix
{
public:
	struct StatMod
	{
		int typeId = 0;                                  // affix_template.stat_typeN (1-based spend-id; drives scaling)
		UnitDefines::Stat stat = UnitDefines::NullStat;  // converted Stat (display/output key)
		float value = 0.f;                               // affix_template.stat_valueN (REAL weight)
	};

	ItemAffix() = default;

	int entry() const { return m_entry; }
	const std::string& name() const { return m_name; }
	int minLevel() const { return m_minLevel; }
	int maxLevel() const { return m_maxLevel; }

	// Add a (spend-id, converted-stat, value) tuple in DB column order (slot 1..5).
	void addStat(int typeId, UnitDefines::Stat stat, float value)
	{
		if (stat == UnitDefines::NullStat)
			return;
		m_stats.push_back({ typeId, stat, value });
		m_statOrder.push_back(stat);
	}

	// Ordered list of stats this affix grants (tooltip renders these first, in this order).
	const std::vector<UnitDefines::Stat>& statOrder() const { return m_statOrder; }

	// Raw stat mods (typeId + value) — ItemDefiner::crunchItemStats applies the scaling formula.
	const std::vector<StatMod>& stats() const { return m_stats; }

	// Populated by sItemDefiner during load.
	int m_entry = 0;
	std::string m_name;
	std::string m_nameSingleNoun;
	int m_minLevel = 0;
	int m_maxLevel = 0;

private:
	std::vector<StatMod> m_stats;
	std::vector<UnitDefines::Stat> m_statOrder;
};
