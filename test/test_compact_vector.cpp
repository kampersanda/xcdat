#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <algorithm>
#include <random>

#include "doctest/doctest.h"
#include "test_common.hpp"
#include "xcdat/compact_vector.hpp"

TEST_CASE("Test compact_vector (zero)") {
    std::vector<std::uint64_t> ints = {0, 0, 0, 0, 0};
    xcdat::compact_vector cv(ints);

    REQUIRE_EQ(cv.size(), ints.size());

    for (std::uint64_t i = 0; i < ints.size(); i++) {
        REQUIRE_EQ(cv[i], ints[i]);
    }
}

TEST_CASE("Test compact_vector (tiny)") {
    std::vector<std::uint64_t> ints = {2, 0, 14, 456, 32, 5544, 23};
    xcdat::compact_vector cv(ints);

    REQUIRE_EQ(cv.size(), ints.size());

    for (std::uint64_t i = 0; i < ints.size(); i++) {
        REQUIRE_EQ(cv[i], ints[i]);
    }
}

TEST_CASE("Test compact_vector (random)") {
    std::vector<std::uint64_t> ints = xcdat::test::make_random_ints(10000, 0, UINT16_MAX);
    xcdat::compact_vector cv(ints);

    REQUIRE_EQ(cv.size(), ints.size());

    for (std::uint64_t i = 0; i < ints.size(); i++) {
        REQUIRE_EQ(cv[i], ints[i]);
    }
}
