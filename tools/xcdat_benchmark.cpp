#include <chrono>
#include <random>

#include <xcdat.hpp>

#include "cmd_line_parser/parser.hpp"
#include "tinyformat/tinyformat.h"

static constexpr int num_trials = 10;

cmd_line_parser::parser make_parser(int argc, char** argv) {
    cmd_line_parser::parser p(argc, argv);
    p.add("input_keys", "Input filepath of keywords");
    p.add("num_samples", "Number of sample keys for searches (default=1000)", "-n", false);
    p.add("random_seed", "Random seed for sampling (default=13)", "-s", false);
    p.add("binary_mode", "Is binary mode? (default=0)", "-b", false);
    return p;
}

std::vector<std::string_view> sample_keys(const std::vector<std::string>& keys, std::uint64_t num_samples,
                                          std::uint64_t random_seed) {
    std::mt19937_64 engine(random_seed);
    std::uniform_int_distribution<std::uint64_t> dist(0, keys.size() - 1);

    std::vector<std::string_view> sampled_keys(num_samples);
    for (std::uint64_t i = 0; i < num_samples; i++) {
        sampled_keys[i] = std::string_view(keys[dist(engine)]);
    }
    return sampled_keys;
}

template <class Trie>
std::vector<std::uint64_t> extract_ids(const Trie& trie, const std::vector<std::string_view>& keys) {
    std::vector<std::uint64_t> sampled_ids(keys.size());
    for (std::uint64_t i = 0; i < keys.size(); i++) {
        sampled_ids[i] = trie.lookup(keys[i]).value();
    }
    return sampled_ids;
}

template <class Trie>
Trie benchmark_build(const std::vector<std::string>& keys, bool binary_mode) {
    const auto start_tp = std::chrono::high_resolution_clock::now();
    Trie trie(keys, binary_mode);
    const auto stop_tp = std::chrono::high_resolution_clock::now();

    const auto dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop_tp - start_tp);
    const double time_in_sec = dur_ms.count() / 1000.0;
    const double memory_in_bytes = xcdat::memory_in_bytes(trie);

    tfm::printfln("Number of keys: %d", trie.num_keys());
    tfm::printfln("Memory usage in bytes: %d", memory_in_bytes);
    tfm::printfln("Memory usage in MiB: %g", memory_in_bytes / (1024.0 * 1024.0));
    tfm::printfln("Construction time in seconds: %g", time_in_sec);

    return trie;
}

template <class Trie>
void benchmark_lookup(const Trie& trie, const std::vector<std::string_view>& queries) {
    // Warmup
    volatile std::uint64_t tmp = 0;
    for (const auto& query : queries) {
        tmp += trie.lookup(query).value();
    }

    // Measure
    const auto start_tp = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < num_trials; r++) {
        for (const auto& query : queries) {
            tmp += trie.lookup(query).value();
        }
    }
    const auto stop_tp = std::chrono::high_resolution_clock::now();

    const auto dur_us = std::chrono::duration_cast<std::chrono::microseconds>(stop_tp - start_tp);
    const auto elapsed_us = static_cast<double>(dur_us.count());

    tfm::printfln("Lookup time in microsec/query: %g", elapsed_us / (num_trials * queries.size()));
}

template <class Trie>
void benchmark_decode(const Trie& trie, const std::vector<std::uint64_t>& queries) {
    // Warmup
    volatile std::uint64_t tmp = 0;
    for (const std::uint64_t query : queries) {
        tmp += trie.decode(query).size();
    }

    // Measure
    const auto start_tp = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < num_trials; r++) {
        for (const std::uint64_t query : queries) {
            tmp += trie.decode(query).size();
        }
    }
    const auto stop_tp = std::chrono::high_resolution_clock::now();

    const auto dur_us = std::chrono::duration_cast<std::chrono::microseconds>(stop_tp - start_tp);
    const auto elapsed_us = static_cast<double>(dur_us.count());

    tfm::printfln("Decode time in microsec/query: %g", elapsed_us / (num_trials * queries.size()));
}

template <class Trie>
void benchmark(std::vector<std::string> keys, const std::vector<std::string_view>& query_keys, bool binary_mode) {
    const auto trie = benchmark_build<Trie>(keys, binary_mode);
    const auto query_ids = extract_ids(trie, query_keys);

    benchmark_lookup(trie, query_keys);
    benchmark_decode(trie, query_ids);
}

int main(int argc, char** argv) {
#ifndef NDEBUG
    tfm::warnfln("The code is running in debug mode.");
#endif
    std::ios::sync_with_stdio(false);

    auto p = make_parser(argc, argv);
    if (!p.parse()) {
        return 1;
    }

    const auto input_keys = p.get<std::string>("input_keys");
    const auto num_samples = p.get<std::uint64_t>("num_samples", 1000);
    const auto random_seed = p.get<std::uint64_t>("random_seed", 13);
    const auto binary_mode = p.get<bool>("binary_mode", false);

    auto keys = xcdat::load_strings(input_keys);
    if (keys.empty()) {
        tfm::errorfln("Error: The input dataset is empty.");
        return 1;
    }

    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    const auto query_keys = sample_keys(keys, num_samples, random_seed);

    tfm::printfln("** xcdat::trie_7_type **");
    benchmark<xcdat::trie_7_type>(keys, query_keys, binary_mode);

    tfm::printfln("** xcdat::trie_8_type **");
    benchmark<xcdat::trie_8_type>(keys, query_keys, binary_mode);

    return 0;
}