#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <algorithm>
#include <iostream>
#include <random>
#include <string>

#include "doctest/doctest.h"
#include "test_common.hpp"
#include "xcdat.hpp"

void test_basic_search(const xcdat::trie& trie, const std::vector<std::string>& keys,
                       const std::vector<std::string>& others) {
    for (size_t i = 0; i < keys.size(); i++) {
        auto id = trie.lookup(keys[i]);
        REQUIRE(id.has_value());
        REQUIRE_LT(id.value(), keys.size());
        auto decoded = trie.access(id.value());
        REQUIRE_EQ(keys[i], decoded);
    }

    for (size_t i = 0; i < others.size(); i++) {
        auto id = trie.lookup(others[i]);
        REQUIRE_FALSE(id.has_value());
    }
}

TEST_CASE("Test xcdat::trie (tiny)") {
    std::vector<std::string> keys = {"ACML",  "AISTATS", "DS",    "DSAA",   "ICDM",  "ICML",
                                     "PAKDD", "SDM",     "SIGIR", "SIGKDD", "SIGMOD"};
    std::vector<std::string> others = {"PKDD", "ICDE", "SIGSPATIAL", "ACL", "SPIRE"};

    auto trie = xcdat::trie::build(keys);
    test_basic_search(trie, keys, others);
}

TEST_CASE("Test xcdat::trie (random)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(10000, 1, 30));
    auto others = xcdat::test::extract_keys(keys);

    auto trie = xcdat::trie::build(keys);
    test_basic_search(trie, keys, others);
}

TEST_CASE("Test xcdat::trie (keys)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::io::load_strings("keys.txt"));
    auto others = xcdat::test::extract_keys(keys);

    auto trie = xcdat::trie::build(keys);
    test_basic_search(trie, keys, others);
}
