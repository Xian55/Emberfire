#pragma once
#include <cstdint>

// GameObjDefines.h exposes the GoDefines:: namespace (NOT GameObjDefines::). The client uses
// GoDefines::Type::Container (scoped), GoDefines::ToggleState::Open (scoped) AND GoDefines::Open /
// GoDefines::Closed (unscoped), so ToggleState is a plain enum. Type and Flags also referenced.
namespace GoDefines
{
	// GameObject category.
	enum Type
	{
		Container = 0,    // TODO verify
		MapChanger = 1,   // TODO verify (entries 1/4 "Aspen Chest"/"Yellow Orchid" = doodads)
		RespawnNode = 2,  // GROUND TRUTH: DB entry 2 "Waypoint" type 2 = SPAWN/respawn point (184,90).
		                  //   Querying it via op45 makes the server throw -> disconnect.
		Waypoint = 3      // GROUND TRUTH: DB entry 3 "Portal" type 3 = the TRAVEL node steam queried OK.
		                  //   setStandingWaypoint must trigger on THIS (was =2 = the respawn point).
	};

	// Open/closed toggle state. Synced via ObjDefines::Variable::ToggleState.
	enum ToggleState
	{
		Closed = 0, // TODO verify
		Open = 1    // TODO verify
	};

	// GameObject flags bitmask. DEFINITIVE from GameObjTemplateEditor checkbox order (dump_client @0x121142)
	// + DB (Aspen Chest flags=9=ShowName+Lockpicking, Strongbox=25=+Lv2xForLockpick). Editor order = bit order.
	enum Flags
	{
		ShowName = 0x01,
		Uninteractable = 0x02,
		RenderOnFloor = 0x04,
		Lockpicking = 0x08,         // was 0x10 (wrong)
		Lv2xForLockpick = 0x10,     // was 0x20 (wrong)
		NoMouseoverBrighten = 0x20  // was 0x08 (wrong)
	};
}
