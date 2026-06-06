// lua.hpp — C++ include for the single-file minilua (Lua 5.5). The headers below already wrap minilua.h in
// extern "C"; this keeps the conventional lua.hpp entry point that sol/LuaBridge-style code expects.
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
