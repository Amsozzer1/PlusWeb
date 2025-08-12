cmake_minimum_required(VERSION 3.14)
project(PlusWeb VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(GTest REQUIRED)

# PlusWeb library
add_library(PlusWeb
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/HttpServer.cpp
    src/RouteRegistry.cpp
    src/trie.cpp
    src/utils.cpp
)

target_include_directories(PlusWeb PUBLIC include)

# Main executable
add_executable(main main.cpp)
target_link_libraries(main PlusWeb)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)
