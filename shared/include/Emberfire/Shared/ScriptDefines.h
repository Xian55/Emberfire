#pragma once
#include <cstdint>

// Script/gossip action commands. Client references ScriptDefines::CmdTypes::ScriptCmd_X (scoped) and
// stringifies their integer values (to_string), so any ordinal scheme works; values TODO verify.
namespace ScriptDefines
{
	// Plain enum: editors call to_string(ScriptDefines::CmdTypes::X) (needs enum->int conversion).
	enum CmdTypes
	{
		ScriptCmd_CastSpell = 0, // TODO verify all ordinals
		ScriptCmd_KillCredit,
		ScriptCmd_Talk,
		ScriptCmd_MoveTo,
		ScriptCmd_SetNpcFlag,
		ScriptCmd_TeleportTo,
		ScriptCmd_ToggleGameObject,
		ScriptCmd_RemoveAura,
		ScriptCmd_CreateItem,
		ScriptCmd_LocateNpc,
		ScriptCmd_OpenBank,
		ScriptCmd_PromptRespec,
		ScriptCmd_QueueArena
	};
}
