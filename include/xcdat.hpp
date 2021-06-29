#pragma once

#include "xcdat/bc_vector_7.hpp"
#include "xcdat/bc_vector_8.hpp"
#include "xcdat/load_visitor.hpp"
#include "xcdat/mmap_visitor.hpp"
#include "xcdat/save_visitor.hpp"
#include "xcdat/size_visitor.hpp"
#include "xcdat/trie.hpp"

namespace xcdat {

using trie_7_type = trie<bc_vector_7>;
using trie_8_type = trie<bc_vector_8>;

using flag_type = std::remove_const_t<decltype(trie_7_type::l1_bits)>;

template <class Trie>
[[maybe_unused]] Trie mmap(const char* address) {
    mmap_visitor visitor(address);
    flag_type flag;
    visitor.visit(flag);
    XCDAT_THROW_IF(flag != Trie::l1_bits, "The input index type is different.");
    Trie idx;
    visitor.visit(idx);
    return idx;
}

template <class Trie>
[[maybe_unused]] Trie load(std::string_view filepath) {
    load_visitor visitor(filepath);
    flag_type flag;
    visitor.visit(flag);
    XCDAT_THROW_IF(flag != Trie::l1_bits, "The input index type is different.");
    Trie idx;
    visitor.visit(idx);
    return idx;
}

template <class Trie>
[[maybe_unused]] std::uint64_t save(const Trie& idx, std::string_view filepath) {
    save_visitor visitor(filepath);
    visitor.visit(Trie::l1_bits);  // flag
    visitor.visit(const_cast<Trie&>(idx));
    return visitor.bytes();
}

template <class Trie>
[[maybe_unused]] std::uint64_t memory_in_bytes(const Trie& idx) {
    size_visitor visitor;
    visitor.visit(Trie::l1_bits);  // flag
    visitor.visit(const_cast<Trie&>(idx));
    return visitor.bytes();
}

[[maybe_unused]] std::vector<std::string> load_strings(std::string_view filepath) {
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

}  // namespace xcdat
