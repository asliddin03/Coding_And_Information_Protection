cmake_minimum_required(VERSION 3.10)
project(BitPermutation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(demo
        src/main.cpp
        src/permutation.cpp
        ../RC4/core.cpp
        ../RC4/core.h
        ../RC4/server.cpp
        ../RC4/client.cpp
        ../RC4/tests.cpp
        ../../hw_2/crypto.cpp
        ../../hw_2/crypto.h
)

add_executable(permutation_tests
        tests/test_permutation.cpp
        src/permutation.cpp
)

enable_testing()
add_test(NAME permutation_tests COMMAND permutation_tests)