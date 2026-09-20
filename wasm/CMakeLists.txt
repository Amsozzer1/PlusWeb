# WASM contract + its native test.
#
# Native (verifies the logic, no Emscripten needed):
#   cmake -B build -DPLUSWEB_BUILD_WASM=ON && cmake --build build
#   ./build/wasm/pw_wasm_contract_test
#
# WebAssembly:
#   emcmake cmake -B build-wasm -DPLUSWEB_BUILD_WASM=ON
#   cmake --build build-wasm
#   node wasm/test_contract.mjs ../build-wasm/wasm/plusweb.js

# Stamped at configure time, so re-run cmake if you want a fresh commit/time.
find_package(Git QUIET)
set(PLUSWEB_COMMIT "unknown")
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE PLUSWEB_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    execute_process(
        # --untracked-files=no: an out-of-source build directory sitting in the
        # tree is not a modification, and CI would otherwise stamp every build
        # as dirty.
        COMMAND ${GIT_EXECUTABLE} status --porcelain --untracked-files=no
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE PLUSWEB_DIRTY
        ERROR_QUIET)
    if(NOT PLUSWEB_DIRTY STREQUAL "")
        set(PLUSWEB_COMMIT "${PLUSWEB_COMMIT}-dirty")
    endif()
endif()
string(TIMESTAMP PLUSWEB_BUILT_AT "%Y-%m-%dT%H:%M:%SZ" UTC)

set(PW_WASM_DEFS
    PLUSWEB_VERSION="${PROJECT_VERSION}"
    PLUSWEB_COMMIT="${PLUSWEB_COMMIT}"
    PLUSWEB_BUILT_AT="${PLUSWEB_BUILT_AT}"
)

# The contract needs the router and the parser, not the event loop, so it links
# the sources it uses directly rather than the whole PlusWeb library.
set(PW_WASM_SOURCES
    ${CMAKE_SOURCE_DIR}/src/HttpBody.cpp
    ${CMAKE_SOURCE_DIR}/src/HttpRequest.cpp
    ${CMAKE_SOURCE_DIR}/src/HttpResponse.cpp
    ${CMAKE_SOURCE_DIR}/src/HttpParser.cpp
    ${CMAKE_SOURCE_DIR}/src/RouteRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/RoutingBase.cpp
    ${CMAKE_SOURCE_DIR}/src/trie.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
    plusweb_wasm.cpp
)

if(EMSCRIPTEN)
    add_executable(plusweb ${PW_WASM_SOURCES})
    target_compile_definitions(plusweb PRIVATE ${PW_WASM_DEFS})
    target_include_directories(plusweb PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(plusweb PRIVATE nlohmann_json::nlohmann_json llhttp::llhttp)
    set_target_properties(plusweb PROPERTIES
        OUTPUT_NAME plusweb
        SUFFIX ".js"
        LINK_FLAGS "-O3 \
-sMODULARIZE=1 \
-sEXPORT_ES6=1 \
-sEXPORT_NAME=createPlusWeb \
-sALLOW_MEMORY_GROWTH=1 \
-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8'] \
-sEXPORTED_FUNCTIONS=['_pw_version','_pw_reset','_pw_register_route','_pw_dispatch','_pw_parse_request','_pw_bench_dispatch','_pw_dump_trie','_malloc','_free']")
else()
    # Native build of the same translation units, so the contract is testable here.
    add_executable(pw_wasm_contract_test ${PW_WASM_SOURCES} test_contract.cpp)
    target_compile_definitions(pw_wasm_contract_test PRIVATE ${PW_WASM_DEFS})
    target_include_directories(pw_wasm_contract_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(pw_wasm_contract_test
        PRIVATE nlohmann_json::nlohmann_json llhttp::llhttp)
    target_compile_options(pw_wasm_contract_test PRIVATE $<$<CONFIG:Debug>:-O2>)
    add_test(NAME wasm_contract COMMAND pw_wasm_contract_test)
endif()
