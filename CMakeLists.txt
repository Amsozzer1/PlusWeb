cmake_minimum_required(VERSION 3.14)
project(PlusWeb VERSION 0.1.0 LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(PLUSWEB_IS_TOP_LEVEL ON)
else()
    set(PLUSWEB_IS_TOP_LEVEL OFF)
endif()

option(PLUSWEB_BUILD_TESTS "Build the PlusWeb test suite" ${PLUSWEB_IS_TOP_LEVEL})
option(PLUSWEB_BUILD_EXAMPLES "Build the PlusWeb examples" ${PLUSWEB_IS_TOP_LEVEL})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(GNUInstallDirs)

# nlohmann/json appears in public headers (HttpBody, HttpResponse::send), so it is a
# PUBLIC dependency: consumers need it on their include path too.
find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
        GIT_SHALLOW TRUE
    )
    set(JSON_Install ON CACHE INTERNAL "")
    FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(Threads REQUIRED)

add_library(PlusWeb
    src/HttpBody.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/HttpServer.cpp
    src/RouteRegistry.cpp
    src/Router.cpp
    src/RoutingBase.cpp
    src/ThreadPool.cpp
    src/trie.cpp
    src/utils.cpp
)
add_library(PlusWeb::PlusWeb ALIAS PlusWeb)

target_include_directories(PlusWeb PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(PlusWeb PUBLIC cxx_std_17)

target_link_libraries(PlusWeb
    PUBLIC
        nlohmann_json::nlohmann_json
        Threads::Threads
)

if(PLUSWEB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(PLUSWEB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ---- Install / packaging ----

include(CMakePackageConfigHelpers)

install(TARGETS PlusWeb
    EXPORT PlusWebTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT PlusWebTargets
    FILE PlusWebTargets.cmake
    NAMESPACE PlusWeb::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PlusWeb
)

configure_package_config_file(
    cmake/PlusWebConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PlusWebConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PlusWeb
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/PlusWebConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/PlusWebConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/PlusWebConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PlusWeb
)
