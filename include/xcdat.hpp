#pragma once

#include "xcdat/bc_vector_7.hpp"
#include "xcdat/bc_vector_8.hpp"

#include "xcdat/io.hpp"
#include "xcdat/trie.hpp"

namespace xcdat {

using trie_7_type = trie<bc_vector_7>;
using trie_8_type = trie<bc_vector_8>;

template <class Trie, class Strings>
static Trie build(const Strings& keys, bool bin_mode = false) {
    return Trie(trie_builder(keys, Trie::bc_vector_type::l1_bits, bin_mode));
}

template <class Trie>
static Trie load(std::string_view filename) {
    Trie trie;
    essentials::load(trie, filename.data());
    return trie;
}

template <class Trie>
static std::uint64_t save(Trie& trie, std::string_view filename) {
    return essentials::save(trie, filename.data());
}

template <class Trie>
static std::uint64_t get_memory_in_bytes(Trie& trie) {
    return essentials::visit<Trie, essentials::sizer>(trie, "");
}

}  // namespace xcdat
