cmake_minimum_required(VERSION 3.14)
project(PlusWeb VERSION 0.1.0 LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(PLUSWEB_IS_TOP_LEVEL ON)
else()
    set(PLUSWEB_IS_TOP_LEVEL OFF)
endif()

option(PLUSWEB_BUILD_TESTS "Build the PlusWeb test suite" ${PLUSWEB_IS_TOP_LEVEL})
option(PLUSWEB_BUILD_EXAMPLES "Build the PlusWeb examples" ${PLUSWEB_IS_TOP_LEVEL})
option(PLUSWEB_BUILD_BENCH "Build the benchmark/profiling harness in bench/" OFF)
option(PLUSWEB_BUILD_WASM "Build the WebAssembly contract in wasm/" OFF)

if(PLUSWEB_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

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

# The llhttp release tarball ships pre-generated C sources, so building it does
# not require npm.
find_package(llhttp 9 QUIET)
if(NOT llhttp_FOUND)
    FetchContent_Declare(llhttp
        URL "https://github.com/nodejs/llhttp/archive/refs/tags/release/v9.4.3.tar.gz"
    )
    set(LLHTTP_BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(LLHTTP_BUILD_STATIC_LIBS ON CACHE INTERNAL "")
    FetchContent_MakeAvailable(llhttp)
endif()

find_package(Threads REQUIRED)

# libuv and llhttp are private: neither header appears in a public PlusWeb
# header, so consumers do not need them on their include path.
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUV REQUIRED IMPORTED_TARGET libuv>=1.0)

add_library(PlusWeb
    src/HttpBody.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/HttpParser.cpp
    src/HttpServer.cpp
    src/RouteRegistry.cpp
    src/Router.cpp
    src/RoutingBase.cpp
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
    PRIVATE
        PkgConfig::LIBUV
        llhttp::llhttp
)

if(PLUSWEB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(PLUSWEB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(PLUSWEB_BUILD_BENCH)
    add_subdirectory(bench)
endif()

if(PLUSWEB_BUILD_WASM)
    add_subdirectory(wasm)
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
