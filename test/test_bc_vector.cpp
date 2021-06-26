#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <algorithm>
#include <random>

#include "doctest/doctest.h"
#include "test_common.hpp"
#include "xcdat/bc_vector.hpp"

struct bc_unit {
    std::uint64_t base;
    std::uint64_t check;
};

std::vector<bc_unit> make_random_units(std::uint64_t n) {
    static constexpr std::uint64_t seed = 13;

    std::mt19937_64 engine(seed);
    std::uniform_int_distribution<std::uint64_t> dist(0, n - 1);

    std::vector<bc_unit> bc_units(n);
    for (std::uint64_t i = 0; i < n; i++) {
        bc_units[i].base = dist(engine);
        bc_units[i].check = dist(engine);
    }
    return bc_units;
}

xcdat::bit_vector::builder to_bit_vector_builder(const std::vector<bool>& bits) {
    xcdat::bit_vector::builder bvb(bits.size());
    for (std::uint64_t i = 0; i < bits.size(); i++) {
        bvb.set_bit(i, bits[i]);
    }
    return bvb;
}

std::uint64_t get_num_ones(const std::vector<bool>& bits) {
    return std::accumulate(bits.begin(), bits.end(), 0ULL);
}

TEST_CASE("Test xcdat::bc_vector") {
    auto bc_units = make_random_units(10000);
    auto leaf_flags = xcdat::test::make_random_bits(10000, 0.2);

    xcdat::bc_vector bc(bc_units, to_bit_vector_builder(leaf_flags));

    REQUIRE_EQ(bc.num_units(), bc_units.size());
    REQUIRE_EQ(bc.num_leaves(), get_num_ones(leaf_flags));

    for (std::uint64_t i = 0; i < bc.num_units(); i++) {
        REQUIRE_EQ(bc.is_leaf(i), leaf_flags[i]);
        if (leaf_flags[i]) {
            REQUIRE_EQ(bc.link(i), bc_units[i].base);
        } else {
            REQUIRE_EQ(bc.base(i), bc_units[i].base);
        }
        REQUIRE_EQ(bc.check(i), bc_units[i].check);
    }
}