#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <algorithm>
#include <random>

#include <xcdat/bit_vector.hpp>

#include "doctest/doctest.h"

std::vector<bool> make_random_bits(std::uint64_t n) {
    static constexpr std::uint64_t seed = 13;

    std::vector<bool> bits;
    std::mt19937_64 engine(seed);

    for (std::uint64_t i = 0; i < n; i++) {
        bits.push_back(engine() & 1);
    }
    return bits;
}

std::uint64_t get_num_ones(const std::vector<bool>& bits) {
    return std::accumulate(bits.begin(), bits.end(), 0ULL);
}

std::uint64_t rank_naive(const std::vector<bool>& bits, std::uint64_t i) {
    return std::accumulate(bits.begin(), bits.begin() + i, 0ULL);
}

std::uint64_t select_naive(const std::vector<bool>& bits, std::uint64_t n) {
    std::uint64_t i = 0;
    for (; i < bits.size(); i++) {
        if (bits[i]) {
            if (n == 0) {
                break;
            }
            n -= 1;
        }
    }
    return i;
}

TEST_CASE("Test bit_vector::builder with resize") {
    const auto bits = make_random_bits(10000);

    xcdat::bit_vector::builder b;
    b.resize(bits.size());

    REQUIRE_EQ(b.size(), bits.size());

    for (std::uint64_t i = 0; i < bits.size(); i++) {
        b.set_bit(i, bits[i]);
    }
    for (std::uint64_t i = 0; i < bits.size(); i++) {
        REQUIRE_EQ(b[i], bits[i]);
    }
}

TEST_CASE("Test bit_vector::builder with push_back") {
    const auto bits = make_random_bits(10000);

    xcdat::bit_vector::builder b;
    b.reserve(bits.size());

    for (std::uint64_t i = 0; i < bits.size(); i++) {
        b.push_back(bits[i]);
    }

    REQUIRE_EQ(b.size(), bits.size());

    for (std::uint64_t i = 0; i < bits.size(); i++) {
        REQUIRE_EQ(b[i], bits[i]);
    }
}

TEST_CASE("Test bit_vector") {
    const auto bits = make_random_bits(10000);

    xcdat::bit_vector bv;
    {
        xcdat::bit_vector::builder b(bits.size());
        for (std::uint64_t i = 0; i < bits.size(); i++) {
            b.set_bit(i, bits[i]);
        }
        bv.build(b, true, true);
    }

    REQUIRE_EQ(bv.size(), bits.size());
    REQUIRE_EQ(bv.num_ones(), get_num_ones(bits));

    for (std::uint64_t i = 0; i < bits.size(); i++) {
        REQUIRE_EQ(bv[i], bits[i]);
    }

    static constexpr std::uint64_t seed = 17;
    std::mt19937_64 engine(seed);

    {
        std::uniform_int_distribution<std::uint64_t> dist(0, bv.size());
        for (std::uint64_t r = 0; r < 100; r++) {
            const std::uint64_t i = dist(engine);
            REQUIRE_EQ(bv.rank(i), rank_naive(bits, i));
        }
    }
    {
        std::uniform_int_distribution<std::uint64_t> dist(0, bv.num_ones() - 1);
        for (std::uint64_t r = 0; r < 100; r++) {
            const std::uint64_t n = dist(engine);
            REQUIRE_EQ(bv.select(n), select_naive(bits, n));
        }
    }
}