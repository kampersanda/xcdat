#pragma once

#include <tuple>

#include "bit_tools.hpp"

namespace xcdat::utils {

template <std::uint64_t N>
constexpr std::tuple<std::uint64_t, std::uint64_t> decompose(std::uint64_t x) {
    return {x / N, x % N};
}

template <class T = std::uint64_t>
constexpr std::uint64_t words_for_bits(std::uint64_t nbits) {
    constexpr std::uint64_t wbits = sizeof(T) * 8;
    return (nbits + wbits - 1) / wbits;
}

//! Get the number of needed bits for representing integer x
inline std::uint64_t needed_bits(std::uint64_t x) {
    return bit_tools::msb(x) + 1;
}

template <class String>
inline String get_suffix(const String& s, std::uint64_t i) {
    return s.substr(i, s.size() - i);
}

}  // namespace xcdat::utils