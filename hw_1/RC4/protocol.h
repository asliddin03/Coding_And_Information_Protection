#pragma once

#include <cstdint>

constexpr unsigned MAX_SESSIONS = 100;
constexpr unsigned BUFFER_SIZE = 65536;

enum class Command {
    Begin,
    Data,
    End
};

struct Session {
    Command command;
    bool error;

    unsigned keySize;
    unsigned dataSize;

    char key[256];
    std::uint8_t buffer[BUFFER_SIZE];
};

struct SharedMemory {
    Session sessions[MAX_SESSIONS];
};