# emberfire::shared — header-only reconstructed wire protocol + game defines.
# Consumers include the headers through the `client/Shared` junction so the headers' internal
# `..\Geo2d.h` (support layer at client/) resolves to a single source. The future server links this.
add_library(emberfire_shared INTERFACE)
add_library(emberfire::shared ALIAS emberfire_shared)

target_compile_features(emberfire_shared INTERFACE cxx_std_17)
target_include_directories(emberfire_shared INTERFACE
    "${CMAKE_SOURCE_DIR}/client/Shared")   # junction -> shared/include/Emberfire/Shared

# Canonical sources (for IDE/source groups). Includes via the junction above, not this path,
# so the headers' relative `..\support.h` includes keep resolving.
file(GLOB EMBERFIRE_SHARED_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Emberfire/Shared/*.h")
target_sources(emberfire_shared INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
    FILES ${EMBERFIRE_SHARED_HEADERS})
