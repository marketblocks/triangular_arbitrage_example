# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
set(CMAKE_CXX_STANDARD 23)

cmake_minimum_required (VERSION 3.8)

project ("triangular_arbitrage_example")

# Include sub-projects.
add_executable (triangular_arbitrage_example "main.cpp" "trinagular_arbitrage.cpp" "triangular_arbitrage.h")

target_link_libraries(triangular_arbitrage_example LINK_PUBLIC deps/CryptoBot_lib)
