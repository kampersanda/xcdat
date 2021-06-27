#pragma once

#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace xcdat::test {

template <class T>
std::vector<T> to_unique_vec(std::vector<T>&& vec) {
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    return std::move(vec);
}

std::vector<bool> make_random_bits(std::uint64_t n, double dens = 0.5, std::uint64_t seed = 13) {
    std::mt19937_64 engine(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    std::vector<bool> bits(n);
    for (std::uint64_t i = 0; i < n; i++) {
        bits[i] = dist(engine) < dens;
    }
    return bits;
}

std::vector<std::uint64_t> make_random_ints(std::uint64_t n, std::uint64_t min, std::uint64_t max,
                                            std::uint64_t seed = 13) {
    std::mt19937_64 engine(seed);
    std::uniform_int_distribution<std::uint64_t> dist(min, max);

    std::vector<std::uint64_t> ints(n);
    for (std::uint64_t i = 0; i < n; i++) {
        ints[i] = dist(engine);
    }
    return ints;
}

std::vector<std::string> make_random_keys(std::uint64_t n, std::uint64_t min_m, std::uint64_t max_m,  //
                                          char min_c = 'A', char max_c = 'Z', std::uint64_t seed = 13) {
    std::mt19937_64 engine(seed);
    std::uniform_int_distribution<std::uint64_t> dist_m(min_m, max_m);
    std::uniform_int_distribution<char> dist_c(min_c, max_c);

    std::vector<std::string> keys(n);
    for (std::uint64_t i = 0; i < n; i++) {
        keys[i].resize(dist_m(engine));
        for (std::uint64_t j = 0; j < keys[i].size(); j++) {
            keys[i][j] = dist_c(engine);
        }
    }
    return keys;
}

std::vector<std::string> extract_keys(std::vector<std::string>& keys, double ratio = 0.1, std::uint64_t seed = 13) {
    std::mt19937_64 engine(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    std::vector<std::string> keys1;
    std::vector<std::string> keys2;

    for (std::uint64_t i = 0; i < keys.size(); ++i) {
        if (ratio < dist(engine)) {
            keys1.push_back(keys[i]);
        } else {
            keys2.push_back(keys[i]);
        }
    }

    keys = keys1;
    return keys2;
}

std::uint64_t max_length(const std::vector<std::string>& keys) {
    std::uint64_t n = 0;
    for (auto& key : keys) {
        n = std::max<std::uint64_t>(n, key.size());
    }
    return n;
}

}  // namespace xcdat::test
