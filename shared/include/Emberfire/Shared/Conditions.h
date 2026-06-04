#pragma once
#include <cstdint>

// Generic condition system (quest/gossip/script gating). Client references Conditions::Type::X
// (scoped, in TableTemplateEditor.cpp). All ordinals TODO verify against server.
namespace Conditions
{
	// Plain enum: editors call to_string(Conditions::Type::X) (needs enum->int conversion).
	enum Type
	{
		Source_Player_LevelGreaterThan = 0, // TODO verify all ordinals
		Source_Player_HasItemInBag,
		Source_Player_HasItemEquipped,
		Source_Player_HasItemBagOrEquipped,
		Source_Unit_HasAura,
		Target_Player_HasItemInBag,
		Target_Player_HasItemEquipped,
		Target_Player_HasItemBagOrEquipped,
		Target_Unit_HasAura,
		Quest_PlayerHasQuest,
		Quest_PlayerHasOrDidQuest,
		Quest_PlayerTurnedInQuest,
		Quest_PlayerHasQuestFinishedInLog,
		Quest_PlayerHasQuestForItem
	};
}
