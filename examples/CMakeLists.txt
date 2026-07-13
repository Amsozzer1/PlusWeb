add_executable(hello_world hello_world/main.cpp)
target_link_libraries(hello_world PRIVATE PlusWeb::PlusWeb)

add_executable(rest_api rest_api/main.cpp)
target_link_libraries(rest_api PRIVATE PlusWeb::PlusWeb)
