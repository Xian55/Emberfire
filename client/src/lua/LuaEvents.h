#pragma once

// Single source of truth for the named events the C++ side fires to Lua (frame:RegisterEvent / OnEvent).
// Listed ONCE in LUA_EVENT_LIST below; both the C++ string constants (namespace LuaEvents) and the Lua-side
// `Events` table (built from this same list in LuaEngine::bindUI) are generated from it, so the names can
// never drift apart. This header is plain (constexpr const char*) — sol-free and client-header-free, so it
// is safe to include from the isolated sol2 TU AND from normal client TUs (Game.cpp / World.cpp).
//
// To add an event: add one X(...) line here, then fire LuaEvents::<NAME> from the matching handler.

#define LUA_EVENT_LIST(X)         \
    X(ADDON_LOADED)               \
    X(PLAYER_LOGIN)               \
    X(PLAYER_TARGET_CHANGED)      \
    X(PLAYER_XP_UPDATE)           \
    X(UNIT_HEALTH)                \
    X(UNIT_MAXHEALTH)             \
    X(UNIT_POWER)                 \
    X(UNIT_MAXPOWER)              \
    X(UNIT_LEVEL)                 \
    X(UNIT_AURA)                  \
    X(UNIT_SPELLCAST_START)       \
    X(UNIT_SPELLCAST_STOP)        \
    X(GROUP_ROSTER_UPDATE)        \
    X(CONTEXT_MENU_CLICK)         \
    X(LOGIN_SHOWN)                \
    X(CHARSELECT_SHOWN)           \
    X(CHARCREATE_SHOWN)           \
    X(WORLD_SHOWN)                \
    X(LOGIN_RESULT)               \
    X(CHARLIST_UPDATE)            \
    X(CHARCREATE_RESULT)          \
    X(UI_POPUP)                   \
    X(UI_POPUP_CLEAR)             \
    X(ZONE_CHANGED)

namespace LuaEvents
{
#define X(name) inline constexpr const char* name = #name;
	LUA_EVENT_LIST(X)
#undef X
}
