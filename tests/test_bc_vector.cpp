#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <algorithm>
#include <random>

#include "doctest/doctest.h"
#include "test_common.hpp"
#include "xcdat/bc_vector_7.hpp"
#include "xcdat/bc_vector_8.hpp"

#ifdef BC_VECTOR_7
using bc_vector_type = xcdat::bc_vector_7;
#elif BC_VECTOR_8
using bc_vector_type = xcdat::bc_vector_8;
#endif

struct bc_unit {
    std::uint64_t base;
    std::uint64_t check;
};

std::vector<bc_unit> make_random_units(std::uint64_t n, std::uint64_t maxv, std::uint64_t seed = 13) {
    std::mt19937_64 engine(seed);
    std::uniform_int_distribution<std::uint64_t> dist(0, maxv);

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

void test_bc_vector(const std::vector<bc_unit>& bc_units, const std::vector<bool>& leaves) {
    bc_vector_type bc(bc_units, to_bit_vector_builder(leaves));

    REQUIRE_EQ(bc.num_units(), bc_units.size());
    REQUIRE_EQ(bc.num_leaves(), get_num_ones(leaves));

    for (std::uint64_t i = 0; i < bc.num_units(); i++) {
        REQUIRE_EQ(bc.is_leaf(i), leaves[i]);
        if (leaves[i]) {
            REQUIRE_EQ(bc.link(i), bc_units[i].base);
        } else {
            REQUIRE_EQ(bc.base(i), bc_units[i].base);
        }
        REQUIRE_EQ(bc.check(i), bc_units[i].check);
    }
}

TEST_CASE("Test bc_vector 10K in [0,10K)") {
    const std::uint64_t size = 10000;
    auto bc_units = make_random_units(size, size - 1);
    auto leaves = xcdat::test::make_random_bits(size, 0.2);
    test_bc_vector(bc_units, leaves);
}

TEST_CASE("Test bc_vector 10K in [0,UINT64_MAX)") {
    const std::uint64_t size = 10000;
    auto bc_units = make_random_units(size, UINT64_MAX);
    auto leaves = xcdat::test::make_random_bits(size, 0.2);
    test_bc_vector(bc_units, leaves);
}
