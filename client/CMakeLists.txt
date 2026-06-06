# emberfire_client — the reverse-engineered Dreadmyst client (x86; static_asserts 32-bit).
# Builds the leaked client sources (src/) against the reconstructed shared layer (via the client/Shared
# junction) + the parent support layer (this dir: Logger.h/Geo2d.h/... , SqlConnector/, Detours/, nlohmann/).

file(GLOB EMBERFIRE_CLIENT_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/lua/*.cpp")
# Vendored minilua (Lua 5.5, single-file) — one C TU includes the amalgamation with LUA_IMPL.
set(EMBERFIRE_LUA_SRC "${CMAKE_CURRENT_SOURCE_DIR}/deps/minilua/minilua_impl.c")
# Stocks.cpp = standalone easter-egg, nothing references it (the in-game editors ARE referenced, keep them).
list(FILTER EMBERFIRE_CLIENT_SRC EXCLUDE REGEX "(Stocks)\\.cpp$")

add_executable(emberfire_client WIN32          # WIN32 = uses WinMain
    ${EMBERFIRE_CLIENT_SRC}
    ${EMBERFIRE_LUA_SRC}
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/sqlite/sqlite3.c"
)

target_link_libraries(emberfire_client PRIVATE emberfire::shared)

# IMPORTANT: do NOT put this dir (client/) on the include path — it holds Math.h, which (Windows is
# case-insensitive) would hijack the system <math.h> and break <cmath>. The client's `..\X.h` includes
# resolve via per-file geometry (src/../Shared -> the junction; src/../Logger.h -> client/Logger.h).
target_include_directories(emberfire_client PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/sqlite"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/zip"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/stubs"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/minilua"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/luabridge"
    "${EMBERFIRE_SFML_ROOT}/include"
)

# Precompiled header: every one of the 112 .cpp includes stdafx.h (windows.h + SFML + STL). Compiling
# it once instead of per-TU is the biggest build-speed win here. (stdafx.h has #pragma once, and the
# files already include it, so CMake's force-include is harmless.) Per-language: not applied to sqlite3.c.
target_precompile_headers(emberfire_client PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/stdafx.h")
# stdafx.h is C++ (SFML/STL); never force it onto the C amalgamation.
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/deps/sqlite/sqlite3.c" ${EMBERFIRE_LUA_SRC}
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

# LuaEngine.cpp is the LuaBridge3 boundary: it needs std::byte (the client disables it via _HAS_STD_BYTE=0
# to avoid a bare `byte` clash under `using namespace std`). LuaBridge uses std::byte, so compile this TU
# isolated — no client PCH, byte re-enabled, /bigobj (the amalgamated LuaBridge + minilua decls are large).
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/lua/LuaEngine.cpp" PROPERTIES
    SKIP_PRECOMPILE_HEADERS ON
    COMPILE_OPTIONS "/U_HAS_STD_BYTE;/D_HAS_STD_BYTE=1;/bigobj")

# Force-include the NTSTATUS shim (Machinespecs.h uses NTSTATUS without <winternl.h>).
target_compile_options(emberfire_client PRIVATE /FIdmyst_ntstatus.h /MP /Zi /bigobj)  # /bigobj: minilua amalgamation + LuaBridge TU are large
target_link_options(emberfire_client PRIVATE /DEBUG)

# No WIN32_LEAN_AND_MEAN — the client relies on full windows.h (timeBeginPeriod from mmsystem).
target_compile_definitions(emberfire_client PRIVATE
    NOMINMAX _CRT_SECURE_NO_WARNINGS _HAS_STD_BYTE=0
    EMBERFIRE_LUA_ENGINE_LUA55     # Lua engine seam (minilua / PUC Lua 5.5; swappable to LuaJIT/Luau later)
    DMYST_NO_LOADING_POPUP)        # skip the 300s "Loading" fallback overlays that gate the UI

target_link_directories(emberfire_client PRIVATE
    "${EMBERFIRE_SFML_ROOT}/lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/zip"
)
target_link_libraries(emberfire_client PRIVATE
    sfml-graphics sfml-window sfml-audio sfml-network sfml-system
    zip winhttp ws2_32 winmm)

# Runtime: copy SFML + zip DLLs next to the exe (zip.dll is the leaked runtime lib, not tracked).
add_custom_command(TARGET emberfire_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${EMBERFIRE_SFML_ROOT}/bin" "$<TARGET_FILE_DIR:emberfire_client>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${EMBERFIRE_DEPS_DIR}/zip/zip.dll" "$<TARGET_FILE_DIR:emberfire_client>"
    # Built-in Lua default UI (client/ui -> runtime ui/) is junctioned by provision.ps1 (below) instead of
    # copied, so source edits are live and in-game /reload picks them up without a rebuild.
    # Provision assets so the build-dir exe runs in place (junctions + config/db; idempotent).
    COMMAND powershell -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/scripts/provision.ps1"
            -ExeDir "$<TARGET_FILE_DIR:emberfire_client>" -GameDir "${EMBERFIRE_GAME_DIR}"
)
