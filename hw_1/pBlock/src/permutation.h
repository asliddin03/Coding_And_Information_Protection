#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bits {

    using Bytes = std::vector<std::uint8_t>;

    enum class BitOrder {
        MostSignificantFirst,
        LeastSignificantFirst
    };

    enum class IndexBase {
        Zero,
        One
    };

    Bytes permute(
        const Bytes& input,
        const std::vector<std::size_t>& p,
        BitOrder order = BitOrder::MostSignificantFirst,
        IndexBase base = IndexBase::Zero
    );

}