#pragma once

#include <tuple>

#include "bit_tools.hpp"

namespace xcdat::utils {

template <std::uint64_t N>
constexpr std::tuple<std::uint64_t, std::uint64_t> decompose(std::uint64_t x) {
    return std::make_tuple(x / N, x % N);
}

template <class T = std::uint64_t>
constexpr std::uint64_t words_for_bits(std::uint64_t nbits) {
    constexpr std::uint64_t wbits = sizeof(T) * 8;
    return (nbits + wbits - 1) / wbits;
}

inline std::uint64_t bits_for_int(std::uint64_t x) {
    return (x > 1) ? bit_tools::msb(x - 1) + 1 : 0;
}

}  // namespace xcdat::utils