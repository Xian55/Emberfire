cmake_minimum_required(VERSION 3.21)

project(Emberfire LANGUAGES CXX C)

# ---------------------------------------------------------------------------
# Emberfire — reverse-engineered Dreadmyst client (+ shared protocol, future server).
# Configure with a preset:  cmake --preset x86-release   (the client static_asserts 32-bit)
# Build:                     cmake --build --preset x86-release
# Test:                      ctest --preset x86-release
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# NB: do NOT add cmake/ to CMAKE_MODULE_PATH — on case-insensitive Windows our cmake/googletest.cmake
# would shadow CMake's built-in GoogleTest module (gtest_discover_tests). We include our helpers by path.

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(WARNING
        "Emberfire's client target is 32-bit (it static_asserts x86). "
        "Configure with the x86 preset:  cmake --preset x86-release")
endif()

option(BUILD_TESTING "Build the unit tests" ON)

# The leaked client + the shared headers use hard-coded `..\Shared\X.h` includes that assume
# Shared is a sibling of the .cpp parent. We keep the canonical headers under shared/ and expose
# them to the client via a `client/Shared` junction (same trick the original project used).
include(cmake/shared_link.cmake)

include(cmake/dependencies.cmake)

add_subdirectory(shared)
add_subdirectory(client)
add_subdirectory(server)

if(BUILD_TESTING)
    enable_testing()
    include(cmake/googletest.cmake)
    add_subdirectory(shared/tests)
    add_subdirectory(client/tests)
    add_subdirectory(server/tests)
endif()
