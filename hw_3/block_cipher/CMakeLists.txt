cmake_minimum_required(VERSION 3.16)
project(crypto_work LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(crypto crypto.cpp)
target_link_libraries(crypto PUBLIC Threads::Threads)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE crypto)

add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE crypto)

enable_testing()
add_test(NAME crypto_tests COMMAND tests)