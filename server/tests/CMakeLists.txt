# Server tests: scaffold for the future from-scratch server (see server/README.md).
add_executable(emberfire_server_tests test_placeholder.cpp)
target_link_libraries(emberfire_server_tests PRIVATE emberfire::shared GTest::gtest_main)
target_compile_definitions(emberfire_server_tests PRIVATE
    NOMINMAX _CRT_SECURE_NO_WARNINGS _HAS_STD_BYTE=0)
gtest_discover_tests(emberfire_server_tests)
