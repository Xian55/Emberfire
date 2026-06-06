/* Shim so consumers expecting the stock <lua.h> get the single-file minilua (Lua 5.5) API with C linkage. */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "minilua.h"
#ifdef __cplusplus
}
#endif
