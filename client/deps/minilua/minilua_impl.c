/* Single translation unit that compiles the entire Lua 5.5 library (edubart/minilua amalgamation).
   Everywhere else, including "minilua.h" without LUA_IMPL yields just the API declarations. */
#define LUA_IMPL
#include "minilua.h"
