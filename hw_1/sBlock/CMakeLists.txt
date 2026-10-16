cmake_minimum_required(VERSION 3.10)
project(SBlock LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(demo src/main.cpp src/substitution.cpp)
add_executable(substitution_tests
        tests/test_substitution.cpp
        src/substitution.cpp
)

enable_testing()
add_test(NAME substitution_tests COMMAND substitution_tests)