cmake_minimum_required(VERSION 3.14)
project(PlusWeb VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PlusWeb library
add_library(PlusWeb
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/HttpServer.cpp
    src/RouteRegistry.cpp
    src/trie.cpp
    src/utils.cpp
)

target_include_directories(PlusWeb PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Installation targets
install(TARGETS PlusWeb
    EXPORT PlusWebTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT PlusWebTargets
    FILE PlusWebTargets.cmake
    NAMESPACE PlusWeb::
    DESTINATION lib/cmake/PlusWeb
)

# Package config files
include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/PlusWebConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PlusWebConfig.cmake
    INSTALL_DESTINATION lib/cmake/PlusWeb
)

write_basic_package_version_file(
    PlusWebConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/PlusWebConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/PlusWebConfigVersion.cmake
    DESTINATION lib/cmake/PlusWeb
)