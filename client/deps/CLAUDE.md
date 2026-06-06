# client/deps/ — vendored third-party

Self-contained third-party sources the client compiles/links directly (so the build needs no package
manager). Each lib is committed here in source/header form; runtime DLLs are NOT tracked (see below).
Wiring lives in `client/CMakeLists.txt` (include dirs + which `.c`/`.h` join the target).

| dir | what | how it's built |
|-----|------|----------------|
| `minilua/` | **Lua 5.5.0** VM, single-file (edubart/minilua amalgamation) | one C TU `minilua_impl.c` |
| `luabridge/` | **LuaBridge3** (kunitoki) C++↔Lua binding, single header `LuaBridge.h` | header-only |
| `sqlite/` | SQLite amalgamation (`sqlite3.c` + `sqlite3.h`) | compiled as C, SKIP_PCH |
| `stubs/` | shim headers for SDKs the client refs but we don't ship | force-included / on include path |
| `zip/` | `zip.h` + `zip.lib` (import lib) for the leaked `zip.dll` | link import lib; DLL copied at runtime |

## minilua (Lua 5.5) — the VM
- `minilua.h` is the whole Lua library (declarations always; full impl only when `LUA_IMPL` is defined).
- `minilua_impl.c` is the SINGLE translation unit that defines `LUA_IMPL` and includes it → compiles all of
  Lua once, as C. Listed in CMake as `EMBERFIRE_LUA_SRC` with `SKIP_PRECOMPILE_HEADERS ON` (it's C, not C++).
- **Shim headers** `lua.h` / `lualib.h` / `lauxlib.h` each = `extern "C" { #include "minilua.h" }`; `lua.hpp`
  pulls all three. They exist so C++ code (LuaBridge, `#include <lua.hpp>`) resolves the conventional stock
  Lua header names against the single-file VM, with correct C linkage.
- **Lua 5.5 vs 5.4 delta that matters:** `lua_newstate(f, ud, seed)` gained a 3rd seed arg (use
  `luaL_makeseed(nullptr)`); everything else the client uses is unchanged.

## luabridge (LuaBridge3) — the binding layer
- Single header. **Include `<lua.hpp>` BEFORE `<LuaBridge.h>`** — it `#error`s if Lua headers aren't present.
- Used by EXACTLY ONE TU: `client/src/lua/LuaEngine.cpp` (the sole binding boundary). See that file + the
  `lua-addon-layer` memory for the patterns (captureless lambdas, `FrameHandle* self`, raw cfunction for
  variadic `SetPoint`, `LUABRIDGE_HAS_EXCEPTIONS 0`).
- **Build gotcha (load-bearing):** LuaBridge uses `std::byte`, but the client globally sets `_HAS_STD_BYTE=0`
  (a bare `byte` clashes under `using namespace std`). So `LuaEngine.cpp` is compiled ISOLATED in CMake
  (`SKIP_PRECOMPILE_HEADERS ON` + `/U_HAS_STD_BYTE;/D_HAS_STD_BYTE=1;/bigobj`) and must not include stdafx.h
  or heavy client headers. Any future binding-using TU needs the same isolation.

## sqlite
SQLite amalgamation. `sqlite3.c` is on the target with `SKIP_PRECOMPILE_HEADERS ON` (C, not the C++ PCH).
The reconstructed `client/SqlConnector/` wraps it.

## stubs
Headers for things the leaked client `#include`s but we neither have nor need at runtime:
`steam_api.h`, `nvapi.h` + `NvApiDriverSettings.h`, and `dmyst_ntstatus.h` (NTSTATUS shim — `Machinespecs.h`
uses NTSTATUS without `<winternl.h>`; force-included via `/FIdmyst_ntstatus.h`). On the include path.

## zip
`zip.h` + `zip.lib` (import library) for the leaked `zip.dll`. The DLL itself is the runtime lib and is NOT
tracked here — it comes from `EMBERFIRE_DEPS_DIR/zip/zip.dll` and is copied next to the exe by the client's
POST_BUILD step.

## Rules
- Don't add `client/` to the include path (`client/Math.h` would hijack `<math.h>` — see `client/CLAUDE.md`).
- Vendored sources are committed; runtime DLLs (SFML, `zip.dll`) and the game runtime are NOT (`.gitignore`).
- Bumping a dep = replace the file(s) here + verify the build; minilua/LuaBridge are single-header drop-ins.
