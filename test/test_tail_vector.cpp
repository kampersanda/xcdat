#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <algorithm>
#include <random>

#include "doctest/doctest.h"
#include "test_common.hpp"
#include "xcdat/tail_vector.hpp"

void test_tail_vector(const std::vector<std::string>& sufs, bool bin_mode = false) {
    xcdat::tail_vector tvec;
    std::vector<std::uint64_t> idxs(sufs.size());

    {
        xcdat::tail_vector::builder tvb;
        for (std::uint64_t i = 0; i < sufs.size(); i++) {
            tvb.set_suffix(sufs[i], i);
        }
        tvb.complete(bin_mode, [&](std::uint64_t npos, std::uint64_t tpos) { idxs[npos] = tpos; });
        tvec = xcdat::tail_vector(tvb);
    }

    for (std::uint64_t i = 0; i < sufs.size(); i++) {
        REQUIRE(tvec.match(sufs[i], idxs[i]));
    }
    for (std::uint64_t i = 0; i < sufs.size(); i++) {
        std::string decoded;
        tvec.decode(idxs[i], [&](char c) { decoded.push_back(c); });
        REQUIRE_EQ(sufs[i], decoded);
    }
}

TEST_CASE("Test xcdat::tail_vector (tiny)") {
    std::vector<std::string> sufs = {"ML", "STATS", "A", "M", "L", "AKDD", "M", "R", "DD", "OD"};
    test_tail_vector(sufs);
}

TEST_CASE("Test xcdat::tail_vector (random, A--B)") {
    std::vector<std::string> sufs = xcdat::test::make_random_keys(10000, 1, 30, 'A', 'B');
    test_tail_vector(sufs);
}

TEST_CASE("Test xcdat::tail_vector (random, A--Z)") {
    std::vector<std::string> sufs = xcdat::test::make_random_keys(10000, 1, 30, 'A', 'Z');
    test_tail_vector(sufs);
}

TEST_CASE("Test xcdat::tail_vector (random, 0x00--0xFF)") {
    std::vector<std::string> sufs = xcdat::test::make_random_keys(10000, 1, 30, INT8_MIN, INT8_MAX);
    test_tail_vector(sufs, true);
}
