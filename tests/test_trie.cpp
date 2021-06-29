#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <algorithm>
#include <iostream>
#include <random>
#include <string>

#include "doctest/doctest.h"
#include "test_common.hpp"
#include "xcdat.hpp"

using trie_type = xcdat::trie_7_type;
// using trie_type = xcdat::trie_8_type;

void test_basic_operations(const trie_type& trie, const std::vector<std::string>& keys,
                           const std::vector<std::string>& others) {
    REQUIRE_EQ(trie.num_keys(), keys.size());
    REQUIRE_EQ(trie.max_length(), xcdat::test::max_length(keys));

    for (std::uint64_t i = 0; i < keys.size(); i++) {
        auto id = trie.lookup(keys[i]);
        REQUIRE(id.has_value());
        REQUIRE_LT(id.value(), keys.size());
        auto decoded = trie.decode(id.value());
        REQUIRE_EQ(keys[i], decoded);
    }

    for (std::uint64_t i = 0; i < others.size(); i++) {
        auto id = trie.lookup(others[i]);
        REQUIRE_FALSE(id.has_value());
    }
}

void test_prefix_search(const trie_type& trie, const std::vector<std::string>& keys,
                        const std::vector<std::string>& others) {
    for (auto& key : keys) {
        size_t num_results = 0;
        auto itr = trie.make_prefix_iterator(key);

        while (itr.next()) {
            const auto id = itr.id();
            const auto decoded = itr.decoded_view();

            REQUIRE_LE(decoded.size(), key.size());
            REQUIRE_EQ(id, trie.lookup(decoded));
            REQUIRE_EQ(decoded, trie.decode(id));

            num_results += 1;
        }

        REQUIRE_LE(1, num_results);
        REQUIRE_LE(num_results, key.size());
    }

    for (auto& key : others) {
        size_t num_results = 0;
        auto itr = trie.make_prefix_iterator(key);

        while (itr.next()) {
            const auto id = itr.id();
            const auto decoded = itr.decoded_view();

            REQUIRE_LT(decoded.size(), key.size());
            REQUIRE_EQ(id, trie.lookup(decoded));
            REQUIRE_EQ(decoded, trie.decode(id));

            num_results += 1;
        }

        REQUIRE_LT(num_results, key.size());
    }
}

void test_predictive_search(const trie_type& trie, const std::vector<std::string>& keys,
                            const std::vector<std::string>& others) {
    for (auto& key : keys) {
        size_t num_results = 0;
        auto itr = trie.make_predictive_iterator(key);

        while (itr.next()) {
            const auto id = itr.id();
            const auto decoded = itr.decoded_view();

            REQUIRE_LE(key.size(), decoded.size());
            REQUIRE_EQ(id, trie.lookup(decoded));
            REQUIRE_EQ(decoded, trie.decode(id));

            num_results += 1;
        }

        REQUIRE_LE(1, num_results);
    }

    for (auto& key : others) {
        auto itr = trie.make_predictive_iterator(key);

        while (itr.next()) {
            const auto id = itr.id();
            const auto decoded = itr.decoded_view();

            REQUIRE_LT(key.size(), decoded.size());
            REQUIRE_EQ(id, trie.lookup(decoded));
            REQUIRE_EQ(decoded, trie.decode(id));
        }
    }
}

void test_enumerate(const trie_type& trie, const std::vector<std::string>& keys) {
    auto itr = trie.make_enumerative_iterator();
    for (auto& key : keys) {
        REQUIRE(itr.next());
        REQUIRE_EQ(itr.decoded_view(), key);
        REQUIRE_EQ(itr.id(), trie.lookup(key));
    }
    REQUIRE_FALSE(itr.next());
}

TEST_CASE("Test trie_type (tiny)") {
    std::vector<std::string> keys = {
        "AirPods",  "AirTag",  "Mac",  "MacBook", "MacBook_Air", "MacBook_Pro",
        "Mac_Mini", "Mac_Pro", "iMac", "iPad",    "iPhone",      "iPhone_SE",
    };
    std::vector<std::string> others = {
        "Google_Pixel", "iPad_mini", "iPadOS", "iPod", "ThinkPad",
    };

    trie_type trie(keys);
    REQUIRE_FALSE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    {
        auto itr = trie.make_prefix_iterator("MacBook_Pro");
        std::vector<std::string> expected = {"Mac", "MacBook", "MacBook_Pro"};
        for (const auto& exp : expected) {
            REQUIRE(itr.next());
            REQUIRE_EQ(itr.decoded(), exp);
            REQUIRE_EQ(itr.id(), trie.lookup(exp));
        }
        REQUIRE_FALSE(itr.next());
    }
    {
        auto itr = trie.make_predictive_iterator("MacBook");
        std::vector<std::string> expected = {"MacBook", "MacBook_Air", "MacBook_Pro"};
        for (const auto& exp : expected) {
            REQUIRE(itr.next());
            REQUIRE_EQ(itr.decoded(), exp);
            REQUIRE_EQ(itr.id(), trie.lookup(exp));
        }
        REQUIRE_FALSE(itr.next());
    }
    {
        auto itr = trie.make_enumerative_iterator();
        for (const auto& key : keys) {
            REQUIRE(itr.next());
            REQUIRE_EQ(itr.decoded(), key);
            REQUIRE_EQ(itr.id(), trie.lookup(key));
        }
        REQUIRE_FALSE(itr.next());
    }
}

TEST_CASE("Test trie_type (real)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::io::load_strings("keys.txt"));
    auto others = xcdat::test::extract_keys(keys);

    trie_type trie(keys);
    REQUIRE_FALSE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, others);
    test_predictive_search(trie, keys, others);
    test_enumerate(trie, keys);
}

TEST_CASE("Test trie_type (random 10K, A--B)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(10000, 1, 30, 'A', 'B'));
    auto others = xcdat::test::extract_keys(keys);

    trie_type trie(keys);
    REQUIRE_FALSE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, others);
    test_predictive_search(trie, keys, others);
    test_enumerate(trie, keys);
}

TEST_CASE("Test trie_type (random 10K, A--Z)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(10000, 1, 30, 'A', 'Z'));
    auto others = xcdat::test::extract_keys(keys);

    trie_type trie(keys);
    REQUIRE_FALSE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, others);
    test_predictive_search(trie, keys, others);
    test_enumerate(trie, keys);
}

TEST_CASE("Test trie_type (random 10K, 0x00--0xFF)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(10000, 1, 30, INT8_MIN, INT8_MAX));
    auto others = xcdat::test::extract_keys(keys);

    trie_type trie(keys);
    REQUIRE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, others);
    test_predictive_search(trie, keys, others);
    test_enumerate(trie, keys);
}

#ifdef NDEBUG
TEST_CASE("Test trie_type (random 100K, A--B)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(100000, 1, 30, 'A', 'B'));
    auto others = xcdat::test::extract_keys(keys);

    trie_type trie(keys);
    REQUIRE_FALSE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, others);
    test_predictive_search(trie, keys, others);
    test_enumerate(trie, keys);
}

TEST_CASE("Test trie_type (random 100K, A--Z)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(100000, 1, 30, 'A', 'Z'));
    auto others = xcdat::test::extract_keys(keys);

    trie_type trie(keys);
    REQUIRE_FALSE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, others);
    test_predictive_search(trie, keys, others);
    test_enumerate(trie, keys);
}

TEST_CASE("Test trie_type (random 100K, 0x00--0xFF)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(100000, 1, 30, INT8_MIN, INT8_MAX));
    auto others = xcdat::test::extract_keys(keys);

    trie_type trie(keys);
    REQUIRE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, others);
    test_predictive_search(trie, keys, others);
    test_enumerate(trie, keys);
}
#endif