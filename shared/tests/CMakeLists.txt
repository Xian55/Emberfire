# High-ROI unit tests: lock the reverse-engineered wire facts so regressions are caught.
add_executable(emberfire_shared_tests
    test_worlderror.cpp
    test_attributes.cpp
    test_item.cpp
    test_opcodes.cpp
)
target_link_libraries(emberfire_shared_tests PRIVATE emberfire::shared GTest::gtest_main)
# Same defs the client builds the shared headers with.
target_compile_definitions(emberfire_shared_tests PRIVATE
    NOMINMAX _CRT_SECURE_NO_WARNINGS _HAS_STD_BYTE=0)
gtest_discover_tests(emberfire_shared_tests)
