#pragma once

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

inline bool compareFiles(const std::string& first,
                         const std::string& second) {
    std::ifstream a(first, std::ios::binary);
    std::ifstream b(second, std::ios::binary);

    if (!a || !b)
        throw std::runtime_error("Не удалось открыть файлы");

    char x[4096], y[4096];

    do {
        a.read(x, sizeof x);
        b.read(y, sizeof y);

        if (a.bad() || b.bad())
            throw std::runtime_error("Ошибка чтения файлов");

        if (a.gcount() != b.gcount() ||
            !std::equal(x, x + a.gcount(), y))
            return false;

    } while (a.gcount());

    return true;
}