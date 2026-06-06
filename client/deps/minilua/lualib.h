/* Shim: stock <lualib.h> -> single-file minilua (Lua 5.5). minilua.h defines the whole API; include guarded. */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "minilua.h"
#ifdef __cplusplus
}
#endif
