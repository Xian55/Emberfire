#pragma once
#include <cstdint>

namespace QuestDefines
{
	// Quest objective tally type. Server_QuestTally.m_type cast/compared here; client uses unscoped
	// QuestDefines::TallyNpc etc. in switch-case -> plain enum. Values TODO verify.
	enum TallyType
	{
		TallyNpc = 0,     // TODO verify (kill credit)
		TallyItem = 1,    // TODO verify (gather)
		TallyGameObj = 2, // TODO verify (use object)
		TallySpell = 3    // TODO verify (cast spell)
	};

	// quest flags bitmask. Bit positions TODO verify.
	enum Flags
	{
		QuestFlagRepeatable = 0x01 // TODO verify
	};
}
