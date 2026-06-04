# client/ — the rebuilt C++ client

Reverse-engineered MMO client. `src/` holds the C++ sources; the **reconstructed support layer** lives at
this dir's root (`Logger.h`, `Geo2d.h`, `Math.h`, `Rand.h`, `Files.h`, `StringHelpers.h`, `Vector.h`,
`CrashHandler.h`, + `SqlConnector/`, `Detours/`, `nlohmann/`). Vendored third-party in `deps/`
(`stubs/` steam/nvapi/NTSTATUS, `sqlite/` amalgamation, `zip/` header+import-lib).

## Why the layout is shaped this way (do not "tidy" it)
The sources use hard-coded relative includes — `..\Shared\X.h`, `..\SqlConnector\X.h`, `..\Geo2d.h`, etc.
— that assume one parent dir holding both `Shared/` and the support headers. So:
- The support headers MUST stay at `client/` root (so `src/*.cpp`'s `..\Logger.h` resolves).
- `client/Shared` is a **junction → `../shared/include/Emberfire/Shared`** (created at configure by
  `cmake/shared_link.cmake`; gitignored). This is the single source for the protocol headers and also makes
  the headers' own `..\Geo2d.h` resolve to `client/Geo2d.h`. Same trick the original project used.
- **NEVER add `client/` to the include path.** It contains `Math.h`, which (Windows is case-insensitive)
  would hijack the system `<math.h>` and break `<cmath>`. Includes resolve by per-file geometry, not `/I`.

## Build quirks (in `CMakeLists.txt`, keep them)
x86, C++17, `/FIdmyst_ntstatus.h`, `NOMINMAX _CRT_SECURE_NO_WARNINGS _HAS_STD_BYTE=0 DMYST_NO_LOADING_POPUP`,
no `WIN32_LEAN_AND_MEAN`, `/MP /Zi /DEBUG`, exclude `Stocks.cpp`. **PCH = `src/stdafx.h`** (every TU includes
it; biggest build-speed win) — `sqlite3.c` is excluded via `SKIP_PRECOMPILE_HEADERS` (it's C, not C++).
POST_BUILD copies SFML/zip DLLs + runs `scripts/provision.ps1`.

## Editing the wire protocol
Packet field orders / opcode values / enum bits are reconstructed and easy to get wrong. The ground-truth
values live in `shared/` headers and are cross-checked against `docs/` (binary decompiles) and live proxy
captures. When you touch a packet or enum, update `shared/tests`. Past bugs were almost always a wrong
byte/field-width/enum value — verify against a capture, not intuition.
