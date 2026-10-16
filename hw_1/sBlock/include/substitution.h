#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <vector>

using Bytes = std::vector<std::uint8_t>;
using Table = std::map<std::uint8_t, std::uint8_t>;
using Rule = std::function<std::uint8_t(std::uint8_t)>;

// Замена групп битов по таблице
Bytes substitute(
    const Bytes& input,
    const Table& table,
    unsigned groupBits
);

// Замена групп битов по функции
Bytes substitute(
    const Bytes& input,
    const Rule& rule,
    unsigned groupBits
);