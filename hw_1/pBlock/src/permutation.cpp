#include "permutation.h"

#include <limits>
#include <stdexcept>

namespace bits {

Bytes permute(
    const Bytes& input,
    const std::vector<std::size_t>& p,
    BitOrder order,
    IndexBase base
) {
    if (order != BitOrder::MostSignificantFirst &&
        order != BitOrder::LeastSignificantFirst) {
        throw std::invalid_argument(
            "Неизвестное направление нумерации битов"
        );
    }

    if (base != IndexBase::Zero &&
        base != IndexBase::One) {
        throw std::invalid_argument(
            "Неизвестное начало отсчёта индексов"
        );
    }

    if (input.size() >
        std::numeric_limits<std::size_t>::max() / 8) {
        throw std::length_error(
            "Размер входных данных слишком велик"
        );
    }

    const std::size_t bitCount = input.size() * 8;

    const std::size_t firstIndex =
        base == IndexBase::Zero ? 0 : 1;

    if (p.size() != bitCount) {
        throw std::invalid_argument(
            "Длина P-блока должна совпадать с количеством битов"
        );
    }

    std::vector<bool> used(bitCount, false);

    for (std::size_t index : p) {
        if (index < firstIndex ||
            index - firstIndex >= bitCount) {
            throw std::out_of_range(
                "Индекс в P-блоке выходит за границы входных данных"
            );
        }

        const std::size_t normalized = index - firstIndex;

        if (used[normalized]) {
            throw std::invalid_argument(
                "P-блок содержит повторяющийся индекс"
            );
        }

        used[normalized] = true;
    }

    Bytes result(input.size(), 0);

    for (std::size_t i = 0; i < bitCount; ++i) {
        std::size_t source = p[i] - firstIndex;
        std::size_t destination = i;

        if (order == BitOrder::LeastSignificantFirst) {
            source = bitCount - 1 - source;
            destination = bitCount - 1 - destination;
        }

        const std::size_t sourceByte = source / 8;
        const std::size_t sourceShift = 7 - source % 8;

        const unsigned bit =
            (input[sourceByte] >> sourceShift) & 1u;

        const std::size_t destinationByte = destination / 8;
        const std::size_t destinationShift =
            7 - destination % 8;

        result[destinationByte] |=
            static_cast<std::uint8_t>(
                bit << destinationShift
            );
    }

    return result;
}

}