#pragma once

#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace xcdat::io {

[[maybe_unused]] static std::vector<std::string> load_strings(std::string_view filepath) {
    std::ifstream ifs(filepath);
    if (!ifs) {
        return {};
    }
    std::vector<std::string> strs;
    for (std::string str; std::getline(ifs, str);) {
        strs.push_back(str);
    }
    return strs;
}

template <typename T>
void load_pod(std::istream& is, T& val) {
    static_assert(std::is_pod<T>::value);
    is.read(reinterpret_cast<char*>(&val), sizeof(T));
}

template <typename T, typename Allocator>
void load_vec(std::istream& is, std::vector<T, Allocator>& vec) {
    size_t n;
    load_pod(is, n);
    vec.resize(n);
    is.read(reinterpret_cast<char*>(vec.data()), static_cast<std::streamsize>(sizeof(T) * n));
}

template <typename T>
void save_pod(std::ostream& os, T const& val) {
    static_assert(std::is_pod<T>::value);
    os.write(reinterpret_cast<char const*>(&val), sizeof(T));
}

template <typename T, typename Allocator>
void save_vec(std::ostream& os, std::vector<T, Allocator> const& vec) {
    static_assert(std::is_pod<T>::value);
    size_t n = vec.size();
    save_pod(os, n);
    os.write(reinterpret_cast<char const*>(vec.data()), static_cast<std::streamsize>(sizeof(T) * n));
}

}  // namespace xcdat::io
