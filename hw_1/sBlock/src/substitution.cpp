#include "substitution.h"

#include <limits>
#include <stdexcept>

Bytes substitute(
    const Bytes& input,
    const Rule& rule,
    unsigned groupBits
) {
    if (!rule || groupBits < 1 || groupBits > 8) {
        throw std::invalid_argument("Неверные параметры замены");
    }

    if (input.size() > std::numeric_limits<std::size_t>::max() / 8) {
        throw std::length_error("Слишком большой массив");
    }

    const std::size_t totalBits = input.size() * 8;

    if (totalBits % groupBits != 0) {
        throw std::invalid_argument("В конце массива неполная группа");
    }

    Bytes result(input.size(), 0);

    for (std::size_t start = 0; start < totalBits; start += groupBits) {
        std::uint8_t key = 0;

        // Читаем группу битов и собираем её значение
        for (unsigned i = 0; i < groupBits; ++i) {
            const std::size_t position = start + i;

            const unsigned bit =
                (input[position / 8] >> (7 - position % 8)) & 1u;

            key = static_cast<std::uint8_t>((key << 1) | bit);
        }

        // Получаем замену
        const std::uint8_t value = rule(key);

        if (value >= (1u << groupBits)) {
            throw std::out_of_range("Замена не помещается в группу");
        }

        // Записываем замену на место исходной группы
        for (unsigned i = 0; i < groupBits; ++i) {
            const std::size_t position = start + i;

            const unsigned bit =
                (value >> (groupBits - 1 - i)) & 1u;

            result[position / 8] |=
                static_cast<std::uint8_t>(
                    bit << (7 - position % 8)
                );
        }
    }

    return result;
}

Bytes substitute(
    const Bytes& input,
    const Table& table,
    unsigned groupBits
) {
    const Rule lookup = [&table](std::uint8_t key) -> std::uint8_t {
        const auto it = table.find(key);

        if (it == table.end()) {
            throw std::out_of_range("В таблице нет нужной замены");
        }

        return it->second;
    };

    return substitute(input, lookup, groupBits);
}