#pragma once

#include "xcdat/bc_vector_7.hpp"
#include "xcdat/bc_vector_8.hpp"

#include "xcdat/io.hpp"
#include "xcdat/trie.hpp"

namespace xcdat {

using trie_7_type = trie<bc_vector_7>;
using trie_8_type = trie<bc_vector_8>;

}  // namespace xcdat
