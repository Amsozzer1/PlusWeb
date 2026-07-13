include(FetchContent)

find_package(GTest 1.11 QUIET)
if(NOT GTest_FOUND)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# test_Get.cpp drives a live PlusWeb server over the loopback interface with libcurl.
find_package(CURL REQUIRED)

add_executable(PlusWeb_tests
    test_utils.cpp
    test_Get.cpp
)

target_link_libraries(PlusWeb_tests
    PRIVATE
        PlusWeb::PlusWeb
        GTest::gtest
        GTest::gtest_main
        CURL::libcurl
)

include(GoogleTest)
gtest_discover_tests(PlusWeb_tests)
