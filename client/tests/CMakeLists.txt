# Client tests are a scaffold only: the client is a 32-bit, SFML/WinMain app built around heavy global
# state, so almost nothing is unit-testable without first extracting pure logic into a library. Add real
# tests here as that logic is factored out. (The high-value, pure tests live in shared/tests.)
add_executable(emberfire_client_tests test_placeholder.cpp)
target_link_libraries(emberfire_client_tests PRIVATE emberfire::shared GTest::gtest_main)
target_compile_definitions(emberfire_client_tests PRIVATE
    NOMINMAX _CRT_SECURE_NO_WARNINGS _HAS_STD_BYTE=0)
gtest_discover_tests(emberfire_client_tests)
