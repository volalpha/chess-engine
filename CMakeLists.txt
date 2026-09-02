cmake_minimum_required(VERSION 3.16)

project(chess_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

option(USE_SANITIZERS "Enable ASAN and UBSAN for testing" OFF)
if(USE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_executable(chess_engine
    src/main.cpp
    src/bitboard.cpp
    src/board.cpp
    src/zobrist.cpp
    src/tt.cpp
    src/uci.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(chess_engine PRIVATE Threads::Threads)
target_include_directories(chess_engine PRIVATE include)

add_executable(chess_engine_benchmark
    src/benchmark.cpp
    src/bitboard.cpp
    src/board.cpp
    src/zobrist.cpp
    src/tt.cpp
    src/uci.cpp
)
target_link_libraries(chess_engine_benchmark PRIVATE Threads::Threads)
target_include_directories(chess_engine_benchmark PRIVATE include)

# Testing
enable_testing()

add_executable(chess_engine_tests
    tests/regression_tests.cpp
    src/bitboard.cpp
    src/board.cpp
    src/zobrist.cpp
    src/tt.cpp
    src/uci.cpp
)
target_include_directories(chess_engine_tests PRIVATE include)
target_link_libraries(chess_engine_tests PRIVATE Threads::Threads)
add_test(NAME RegressionTests COMMAND chess_engine_tests)