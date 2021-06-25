#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <algorithm>
#include <random>

#include <xcdat/compact_vector.hpp>

#include "doctest/doctest.h"

std::vector<std::uint64_t> make_random_ints(std::uint64_t n) {
    static constexpr std::uint64_t seed = 13;

    std::mt19937_64 engine(seed);
    std::uniform_int_distribution<std::uint64_t> dist(0, UINT16_MAX);

    std::vector<std::uint64_t> ints(n);
    for (std::uint64_t i = 0; i < n; i++) {
        ints[i] = dist(engine);
    }
    return ints;
}

TEST_CASE("Test xcdat::compact_vector (tiny)") {
    std::vector<std::uint64_t> ints = {2, 0, 14, 456, 32, 5544, 23};
    xcdat::compact_vector cv(ints);

    REQUIRE_EQ(cv.size(), ints.size());

    for (std::uint64_t i = 0; i < ints.size(); i++) {
        REQUIRE_EQ(cv[i], ints[i]);
    }
}

TEST_CASE("Test xcdat::compact_vector (random)") {
    std::vector<std::uint64_t> ints = make_random_ints(10000);
    xcdat::compact_vector cv(ints);

    REQUIRE_EQ(cv.size(), ints.size());

    for (std::uint64_t i = 0; i < ints.size(); i++) {
        REQUIRE_EQ(cv[i], ints[i]);
    }
}
