cmake_minimum_required(VERSION 3.10)
project(BitOperations LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(demo src/main.cpp src/bit_operations.cpp)
add_executable(bit_tests tests/tests.cpp src/bit_operations.cpp)

enable_testing()
add_test(NAME bit_tests COMMAND bit_tests)