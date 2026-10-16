cmake_minimum_required(VERSION 3.10)
project(RC4Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(server server.cpp)
add_executable(client client.cpp)
add_executable(tests tests.cpp)

foreach(target server client tests)
    target_link_libraries(${target} PRIVATE Threads::Threads)
endforeach()

enable_testing()
add_test(NAME component_tests COMMAND tests)

# printf 'Plaintext' > original.txt
# ./build/client crypt original.txt encrypted.bin "Key"
# ./build/client crypt encrypted.bin restored.txt "Key"
# ./build/client compare original.txt restored.txt