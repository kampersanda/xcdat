#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <algorithm>
#include <iostream>
#include <random>
#include <string>

#include "doctest/doctest.h"
#include "mm_file/mm_file.hpp"
#include "test_common.hpp"
#include "xcdat.hpp"

#ifdef TRIE_7
using trie_type = xcdat::trie_7_type;
#define TRIE_NAME "xcdat::trie_7_type"
#elif TRIE_8
using trie_type = xcdat::trie_8_type;
#define TRIE_NAME "xcdat::trie_8_type"
#elif TRIE_15
using trie_type = xcdat::trie_15_type;
#define TRIE_NAME "xcdat::trie_15_type"
#elif TRIE_16
using trie_type = xcdat::trie_16_type;
#define TRIE_NAME "xcdat::trie_16_type"
#endif

std::vector<std::string> load_strings(const std::string& filepath, char delim = '\n') {
    std::ifstream ifs(filepath);
    XCDAT_THROW_IF(!ifs.good(), "Cannot open the input file");

    std::vector<std::string> strs;
    for (std::string str; std::getline(ifs, str, delim);) {
        strs.push_back(str);
    }
    return strs;
}

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
                        const std::vector<std::string>& queries) {
    for (auto& query : queries) {
        std::vector<std::string> results;
        for (auto itr = trie.make_prefix_iterator(query); itr.next();) {
            const auto id = itr.id();
            const auto decoded = itr.decoded_view();

            REQUIRE_LE(decoded.size(), query.size());
            REQUIRE_EQ(id, trie.lookup(decoded));
            REQUIRE_EQ(decoded, trie.decode(id));

            results.push_back(itr.decoded());
        }

        auto naive_results = xcdat::test::prefix_search_naive(keys, query);
        REQUIRE_EQ(results.size(), naive_results.size());

        for (std::size_t i = 0; i < results.size(); i++) {
            REQUIRE_EQ(results[i], naive_results[i]);
        }
    }
}

void test_predictive_search(const trie_type& trie, const std::vector<std::string>& keys,
                            const std::vector<std::string>& queries) {
    for (auto& query : queries) {
        std::string_view query_view{query.c_str(), query.size() / 3 + 1};

        std::vector<std::string> results;
        for (auto itr = trie.make_predictive_iterator(query_view); itr.next();) {
            const auto id = itr.id();
            const auto decoded = itr.decoded_view();

            REQUIRE_LE(query_view.size(), decoded.size());
            REQUIRE_EQ(id, trie.lookup(decoded));
            REQUIRE_EQ(decoded, trie.decode(id));

            results.push_back(itr.decoded());
        }

        auto naive_results = xcdat::test::predictive_search_naive(keys, query_view);
        REQUIRE_EQ(results.size(), naive_results.size());

        for (std::size_t i = 0; i < results.size(); i++) {
            REQUIRE_EQ(results[i], naive_results[i]);
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

void test_io(const trie_type& trie, const std::vector<std::string>& keys, const std::vector<std::string>& others) {
    const char* tmp_filepath = "tmp.idx";

    const std::uint64_t memory = xcdat::memory_in_bytes(trie);
    REQUIRE_EQ(memory, xcdat::save(trie, tmp_filepath));

    {
        const auto loaded = xcdat::load<trie_type>(tmp_filepath);
        REQUIRE_EQ(trie.bin_mode(), loaded.bin_mode());
        REQUIRE_EQ(trie.num_keys(), loaded.num_keys());
        REQUIRE_EQ(trie.alphabet_size(), loaded.alphabet_size());
        REQUIRE_EQ(trie.max_length(), loaded.max_length());
        REQUIRE_EQ(trie.num_nodes(), loaded.num_nodes());
        REQUIRE_EQ(trie.num_units(), loaded.num_units());
        REQUIRE_EQ(trie.num_free_units(), loaded.num_free_units());
        REQUIRE_EQ(trie.tail_length(), loaded.tail_length());
        REQUIRE_EQ(memory, xcdat::memory_in_bytes(loaded));
        test_basic_operations(loaded, keys, others);
    }

    {
        mm::file_source<char> fin(tmp_filepath, mm::advice::sequential);
        const auto mapped = xcdat::mmap<trie_type>(fin.data());
        REQUIRE_EQ(trie.bin_mode(), mapped.bin_mode());
        REQUIRE_EQ(trie.num_keys(), mapped.num_keys());
        REQUIRE_EQ(trie.alphabet_size(), mapped.alphabet_size());
        REQUIRE_EQ(trie.max_length(), mapped.max_length());
        REQUIRE_EQ(trie.num_nodes(), mapped.num_nodes());
        REQUIRE_EQ(trie.num_units(), mapped.num_units());
        REQUIRE_EQ(trie.num_free_units(), mapped.num_free_units());
        REQUIRE_EQ(trie.tail_length(), mapped.tail_length());
        REQUIRE_EQ(memory, xcdat::memory_in_bytes(mapped));
        test_basic_operations(mapped, keys, others);
    }

    std::remove(tmp_filepath);
}

TEST_CASE("Test " TRIE_NAME " (tiny)") {
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

    test_io(trie, keys, others);
}

TEST_CASE("Test " TRIE_NAME " (unsort)") {
    std::vector<std::string> keys = {
        "AirPods",  "AirTag",  "Mac",  "MacBook", "MacBook_Pro", "MacBook_Air",
        "Mac_Mini", "Mac_Pro", "iMac", "iPad",    "iPhone",      "iPhone_SE",
    };

    auto func = [&]() { auto trie = trie_type(keys); };
    REQUIRE_THROWS_AS(func(), const xcdat::exception&);
}

TEST_CASE("Test " TRIE_NAME " (not unique)") {
    std::vector<std::string> keys = {
        "AirPods",  "AirTag",  "Mac",  "MacBook", "MacBook", "MacBook_Pro",
        "Mac_Mini", "Mac_Pro", "iMac", "iPad",    "iPhone",  "iPhone_SE",
    };

    auto func = [&]() { auto trie = trie_type(keys); };
    REQUIRE_THROWS_AS(func(), const xcdat::exception&);
}

TEST_CASE("Test " TRIE_NAME " (real)") {
    auto keys = xcdat::test::to_unique_vec(load_strings("keys.txt"));
    auto others = xcdat::test::extract_keys(keys);
    auto queries = xcdat::test::sample_keys(keys, 100);

    trie_type trie(keys);
    REQUIRE_FALSE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, queries);
    test_predictive_search(trie, keys, queries);
    test_enumerate(trie, keys);
    test_io(trie, keys, others);
}

TEST_CASE("Test " TRIE_NAME " (random 10K, A--B)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(10000, 1, 30, 'A', 'B'));
    auto others = xcdat::test::extract_keys(keys);
    auto queries = xcdat::test::sample_keys(keys, 100);

    trie_type trie(keys);
    REQUIRE_FALSE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, queries);
    test_predictive_search(trie, keys, queries);
    test_enumerate(trie, keys);
    test_io(trie, keys, others);
}

TEST_CASE("Test " TRIE_NAME " (random 10K, A--Z)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(10000, 1, 30, 'A', 'Z'));
    auto others = xcdat::test::extract_keys(keys);
    auto queries = xcdat::test::sample_keys(keys, 100);

    trie_type trie(keys);
    REQUIRE_FALSE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, queries);
    test_predictive_search(trie, keys, queries);
    test_enumerate(trie, keys);
    test_io(trie, keys, others);
}

TEST_CASE("Test " TRIE_NAME " (random 10K, 0x00--0xFF)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(10000, 1, 30, INT8_MIN, INT8_MAX));
    auto others = xcdat::test::extract_keys(keys);
    auto queries = xcdat::test::sample_keys(keys, 100);

    trie_type trie(keys);
    REQUIRE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, queries);
    test_predictive_search(trie, keys, queries);
    test_enumerate(trie, keys);
    test_io(trie, keys, others);
}

#ifdef NDEBUG
TEST_CASE("Test " TRIE_NAME " (random 100K, A--B)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(100000, 1, 30, 'A', 'B'));
    auto others = xcdat::test::extract_keys(keys);
    auto queries = xcdat::test::sample_keys(keys, 1000);

    trie_type trie(keys);
    REQUIRE_FALSE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, queries);
    test_predictive_search(trie, keys, queries);
    test_enumerate(trie, keys);
    test_io(trie, keys, others);
}

TEST_CASE("Test " TRIE_NAME " (random 100K, A--Z)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(100000, 1, 30, 'A', 'Z'));
    auto others = xcdat::test::extract_keys(keys);
    auto queries = xcdat::test::sample_keys(keys, 1000);

    trie_type trie(keys);
    REQUIRE_FALSE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, queries);
    test_predictive_search(trie, keys, queries);
    test_enumerate(trie, keys);
    test_io(trie, keys, others);
}

TEST_CASE("Test " TRIE_NAME " (random 100K, 0x00--0xFF)") {
    auto keys = xcdat::test::to_unique_vec(xcdat::test::make_random_keys(100000, 1, 30, INT8_MIN, INT8_MAX));
    auto others = xcdat::test::extract_keys(keys);
    auto queries = xcdat::test::sample_keys(keys, 1000);

    trie_type trie(keys);
    REQUIRE(trie.bin_mode());

    test_basic_operations(trie, keys, others);
    test_prefix_search(trie, keys, queries);
    test_predictive_search(trie, keys, queries);
    test_enumerate(trie, keys);
    test_io(trie, keys, others);
}
#endif