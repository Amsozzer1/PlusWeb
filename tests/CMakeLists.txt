# Test executable
add_executable(PlusWeb_tests
    test_simple.cpp
    test_utils.cpp
)

target_link_libraries(PlusWeb_tests
    PlusWeb
    GTest::gtest
    GTest::gtest_main
    pthread
)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(PlusWeb_tests)
