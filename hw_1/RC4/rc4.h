#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

class RC4 {
public:
    void setKey(const std::string& key) {
        if (key.empty() || key.size() > 256)
            throw std::runtime_error("Ключ должен содержать 1–256 байтов");

        for (unsigned n = 0; n < 256; ++n)
            state[n] = n;

        unsigned k = 0;

        for (unsigned n = 0; n < 256; ++n) {
            k = (k + state[n] +
                 static_cast<unsigned char>(key[n % key.size()])) % 256;

            std::swap(state[n], state[k]);
        }

        i = j = 0;
        initialized = true;
    }

    void process(std::uint8_t* data, std::size_t size) {
        if (!initialized || (!data && size != 0))
            throw std::runtime_error("RC4: неверные параметры");

        for (std::size_t n = 0; n < size; ++n) {
            i = (i + 1) % 256;
            j = (j + state[i]) % 256;

            std::swap(state[i], state[j]);

            data[n] ^= state[(state[i] + state[j]) % 256];
        }
    }

private:
    std::array<std::uint8_t, 256> state{};
    unsigned i = 0;
    unsigned j = 0;
    bool initialized = false;
};