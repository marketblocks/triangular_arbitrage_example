﻿# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
set(CMAKE_CXX_STANDARD 23)

cmake_minimum_required (VERSION 3.8)

project ("triangular_arbitrage_example")

add_subdirectory("marketblocks")

enable_testing()

# Include sub-projects.
add_executable (triangular_arbitrage_example "main.cpp" "triangular_arbitrage.cpp" "triangular_arbitrage.h"    "sequences.h"  "tri_arb_config.h" "tri_arb_config.cpp")

target_link_libraries(triangular_arbitrage_example marketblocks_lib)
target_include_directories(triangular_arbitrage_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/marketblocks/src)
