# The leaked client uses `#include "..\Shared\X.h"`, and the Shared headers themselves use
# `#include "..\Geo2d.h"` (the support layer). Both assume Shared is a sibling of the parent that
# holds the support headers. We keep the canonical headers under shared/ and expose them to the
# client via a `client/Shared` junction (the same approach the original project used) so everything
# resolves to ONE source with zero edits to the 112 .cpp files.
set(_emberfire_shared_src  "${CMAKE_SOURCE_DIR}/shared/include/Emberfire/Shared")
set(_emberfire_shared_link "${CMAKE_SOURCE_DIR}/client/Shared")

if(NOT EXISTS "${_emberfire_shared_link}")
    if(WIN32)
        file(TO_NATIVE_PATH "${_emberfire_shared_link}" _ln)
        file(TO_NATIVE_PATH "${_emberfire_shared_src}"  _tg)
        execute_process(COMMAND cmd /c mklink /J "${_ln}" "${_tg}"
                        RESULT_VARIABLE _r OUTPUT_QUIET ERROR_QUIET)
        if(NOT _r EQUAL 0)
            message(FATAL_ERROR
                "Could not create the Shared junction:\n  ${_ln}  ->  ${_tg}\n"
                "Create it manually (admin not required):  scripts/bootstrap.ps1")
        endif()
    else()
        file(CREATE_LINK "${_emberfire_shared_src}" "${_emberfire_shared_link}" SYMBOLIC)
    endif()
    message(STATUS "Created Shared junction: ${_emberfire_shared_link} -> ${_emberfire_shared_src}")
endif()
