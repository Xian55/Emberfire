# External deps that are NOT vendored in-repo:
#  - SFML 2.6.1 (x86 prebuilt dev libs) — referenced from EMBERFIRE_DEPS_DIR (verified ABI; not rebuilt).
#  - zip.dll runtime — copied next to the exe at build time (the import lib client/deps/zip/zip.lib is vendored).
# Vendored in-repo (client/deps): sqlite amalgamation, zip header + import lib, steam/nvapi/NTSTATUS stubs.
#
# Paths come from environment variables (no machine-specific paths committed):
#   setx EMBERFIRE_DEPS_DIR  "<dir containing x86/SFML-2.6.1 and zip/zip.dll>"
#   setx EMBERFIRE_GAME_DIR  "<game runtime: content/maps/scripts/game.db>"
# Or override per-configure:  cmake --preset x86-release -D EMBERFIRE_DEPS_DIR=...

set(EMBERFIRE_DEPS_DIR "$ENV{EMBERFIRE_DEPS_DIR}" CACHE PATH
    "Prebuilt deps root (expects x86/SFML-2.6.1 and zip/zip.dll). Defaults to %EMBERFIRE_DEPS_DIR%.")
set(EMBERFIRE_GAME_DIR "$ENV{EMBERFIRE_GAME_DIR}" CACHE PATH
    "Game runtime (content/maps/scripts/game.db, zip.dll) — for running, not building. Defaults to %EMBERFIRE_GAME_DIR%.")

if(NOT EMBERFIRE_DEPS_DIR)
    message(WARNING "EMBERFIRE_DEPS_DIR is empty. Set the env var (setx EMBERFIRE_DEPS_DIR ...) "
                    "or pass -D EMBERFIRE_DEPS_DIR=<dir with x86/SFML-2.6.1>.")
endif()

set(EMBERFIRE_SFML_ROOT "${EMBERFIRE_DEPS_DIR}/x86/SFML-2.6.1")
if(NOT EXISTS "${EMBERFIRE_SFML_ROOT}/include/SFML/Graphics.hpp")
    message(WARNING
        "SFML 2.6.1 (x86) not found under EMBERFIRE_DEPS_DIR=${EMBERFIRE_DEPS_DIR}.\n"
        "Point -D EMBERFIRE_DEPS_DIR=<dir-with x86/SFML-2.6.1> or use vcpkg.json. Client will not link.")
endif()
