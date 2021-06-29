#pragma once

#include "xcdat/bc_vector_7.hpp"
#include "xcdat/bc_vector_8.hpp"

#include "xcdat/io.hpp"
#include "xcdat/trie.hpp"

#include "xcdat/load_visitor.hpp"
#include "xcdat/mmap_visitor.hpp"
#include "xcdat/save_visitor.hpp"
#include "xcdat/size_visitor.hpp"

namespace xcdat {

using trie_7_type = trie<bc_vector_7>;
using trie_8_type = trie<bc_vector_8>;

template <class Trie>
static Trie mmap(const char* address) {
    Trie idx;
    mmap_visitor visitor(address);
    visitor.visit(idx);
    return idx;
}

template <class Trie>
static Trie load(std::string_view filepath) {
    Trie idx;
    load_visitor visitor(filepath);
    visitor.visit(idx);
    return idx;
}

template <class Trie>
static std::uint64_t save(const Trie& idx, std::string_view filepath) {
    save_visitor visitor(filepath);
    visitor.visit(const_cast<Trie&>(idx));
    return visitor.bytes();
}

template <class Trie>
static std::uint64_t memory_in_bytes(const Trie& idx) {
    size_visitor visitor;
    visitor.visit(const_cast<Trie&>(idx));
    return visitor.bytes();
}

}  // namespace xcdat
