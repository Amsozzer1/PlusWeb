# Benchmark and profiling harness. Opt in with -DPLUSWEB_BUILD_BENCH=ON; it is
# off by default so a normal build stays fast and does not need libuv.
#
# Targets:
#   pw_loadgen     closed-loop epoll HTTP load generator (reports starved conns)
#   pw_refserver   bare epoll server; establishes the harness ceiling
#   pw_microbench  in-process pipeline benchmarks with exact allocation counts
#   pw_benchserver PlusWeb server under test

# pw_loadgen and pw_refserver are written directly against epoll, so they are
# Linux-only. The other two are portable, and skipping rather than failing means
# -DPLUSWEB_BUILD_BENCH=ON still gives you the microbenchmarks on a Mac.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(pw_loadgen loadgen.cpp)
    target_link_libraries(pw_loadgen PRIVATE Threads::Threads)

    add_executable(pw_refserver refserver.cpp)
else()
    message(STATUS
        "PlusWeb bench: skipping pw_loadgen and pw_refserver, they need epoll (Linux). "
        "pw_microbench and pw_benchserver will still build.")
endif()

add_executable(pw_microbench microbench.cpp)
target_link_libraries(pw_microbench PRIVATE PlusWeb::PlusWeb)
# HttpParser.h is internal, so the benchmark reaches into src/ for it.
target_include_directories(pw_microbench PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(pw_benchserver benchserver.cpp)
target_link_libraries(pw_benchserver PRIVATE PlusWeb::PlusWeb ${CMAKE_DL_LIBS})

# Optimization matters enormously here; never benchmark a -O0 build.
foreach(t pw_loadgen pw_refserver pw_microbench pw_benchserver)
    if(TARGET ${t})
        target_compile_options(${t} PRIVATE $<$<CONFIG:Debug>:-O2>)
    endif()
endforeach()
