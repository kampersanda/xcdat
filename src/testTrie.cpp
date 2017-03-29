#undef NDEBUG

#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <cstring>

#include "Trie.hpp"

using namespace xcdat;

namespace {

constexpr size_t kNumStrings = 1U << 10;
constexpr size_t kMaxLength = 20;

void to_set(std::vector<std::string>& keys) {
  std::sort(std::begin(keys), std::end(keys));
  keys.erase(std::unique(std::begin(keys), std::end(keys)), std::end(keys));
}

std::string make_key() {
  std::random_device rnd;

  std::string key;
  size_t length = (rnd() % kMaxLength) + 1;
  for (size_t j = 0; j < length; ++j) {
    key += 'A' + (rnd() % 26);
  }

  return key;
}

void make_keys(std::vector<std::string>& keys) {
  keys.clear();
  keys.reserve(kNumStrings);

  for (size_t i = 0; i < kNumStrings; ++i) {
    keys.emplace_back(make_key());
  }

  to_set(keys);
}

void make_other_keys(const std::vector<std::string>& keys, std::vector<std::string>& others) {
  others.clear();

  for (size_t i = 0; i < kNumStrings; ++i) {
    auto string = make_key();
    if (std::find(std::begin(keys), std::end(keys), string) == std::end(keys)) {
      others.push_back(string);
    }
  }

  to_set(others);
}

template <bool Fast>
void test_build(Trie<Fast>& trie, const std::vector<std::pair<const uint8_t*, size_t>>& keys,
                bool binary_mode) {
  std::cerr << "Construction -> build()" << std::endl;

  Trie<Fast>(keys, binary_mode).swap(trie);
  assert(trie.num_keys() == keys.size());
}

template <bool Fast>
void test_basic_operations(const Trie<Fast>& trie,
                           const std::vector<std::pair<const uint8_t*, size_t>>& keys,
                           const std::vector<std::pair<const uint8_t*, size_t>>& others) {
  std::cerr << "Basic operations -> lookup() and access()" << std::endl;

  for (auto& key : keys) {
    const auto id = trie.lookup(key.first, key.second);
    assert(id != kNotFound);
    std::vector<uint8_t> ret;
    assert(trie.access(id, ret));
    assert(ret.size() == key.second);
    assert(std::memcmp(ret.data(), key.first, key.second) == 0);
  }

  for (auto& other : others) {
    const auto id = trie.lookup(other.first, other.second);
    assert(id == kNotFound);
  }
}

template <bool Fast>
void test_prefix_operations(const Trie<Fast>& trie,
                            const std::vector<std::pair<const uint8_t*, size_t>>& keys,
                            const std::vector<std::pair<const uint8_t*, size_t>>& others) {
  std::cerr << "Prefix operations -> common_prefix_lookup()" << std::endl;

  for (auto& key : keys) {
    std::vector<id_type> ids;
    auto num_ids = trie.common_prefix_lookup(key.first, key.second, ids);

    assert(1 <= num_ids);
    assert(num_ids <= kMaxLength);
    assert(num_ids == ids.size());

    for (auto id : ids) {
      std::vector<uint8_t> ret;
      assert(trie.access(id, ret));
      assert(ret.size() <= key.second);
    }
  }

  for (auto& other : others) {
    std::vector<id_type> ids;
    auto num_ids = trie.common_prefix_lookup(other.first, other.second, ids);

    assert(num_ids <= kMaxLength);
    assert(num_ids == ids.size());

    for (auto id : ids) {
      std::vector<uint8_t> ret;
      assert(trie.access(id, ret));
      assert(ret.size() < other.second);
    }
  }
}

template <bool Fast>
void test_predictive_operations(const Trie<Fast>& trie,
                                const std::vector<std::pair<const uint8_t*, size_t>>& keys,
                                const std::vector<std::pair<const uint8_t*, size_t>>& others) {
  std::cerr << "Predictive operations -> predictive_lookup()" << std::endl;

  for (auto& key : keys) {
    std::vector<id_type> ids;
    auto num_ids = trie.predictive_lookup(key.first, key.second, ids);

    assert(1 <= num_ids);
    assert(num_ids == ids.size());

    for (auto id : ids) {
      std::vector<uint8_t> ret;
      assert(trie.access(id, ret));
      assert(key.second <= ret.size());
    }
  }

  for (auto& other : others) {
    std::vector<id_type> ids;
    auto num_ids = trie.predictive_lookup(other.first, other.second, ids);

    assert(num_ids == ids.size());

    for (auto id : ids) {
      std::vector<uint8_t> ret;
      assert(trie.access(id, ret));
      assert(other.second < ret.size());
    }
  }
}

template <bool Fast>
void test_io(const Trie<Fast>& trie) {
  std::cerr << "File I/O -> write() and read()" << std::endl;

  const char* file_name = "test.trie";
  {
    std::ofstream ofs{file_name};
    trie.write(ofs);
  }
  {
    std::ifstream ifs{file_name};
    auto size = static_cast<size_t>(ifs.seekg(0, std::ios::end).tellg());
    assert(size == trie.size_in_bytes());
  }

  Trie<Fast> _trie;
  {
    std::ifstream ifs{file_name};
    _trie.read(ifs);
  }

  assert(trie.num_keys() == _trie.num_keys());
  assert(trie.alphabet_size() == _trie.alphabet_size());
  assert(trie.num_nodes() == _trie.num_nodes());
  assert(trie.num_used_nodes() == _trie.num_used_nodes());
  assert(trie.num_free_nodes() == _trie.num_free_nodes());
  assert(trie.size_in_bytes() == _trie.size_in_bytes());
}

template <bool Fast>
void test_trie(const std::vector<std::pair<const uint8_t*, size_t>>& strings,
               const std::vector<std::pair<const uint8_t*, size_t>>& others) {
  for (int i = 0; i < 2; ++i) {
    std::cerr << "** " << (i % 2 ? "Binary" : "Text") << " Mode **" << std::endl;
    std::cerr << "Testing xcdat::Trie<" << (Fast ? "true" : "false") << ">" << std::endl;
    Trie<Fast> trie;
    test_build(trie, strings, i % 2 == 0);
    test_basic_operations(trie, strings, others);
    test_prefix_operations(trie, strings, others);
    test_predictive_operations(trie, strings, others);
    test_io(trie);
    std::cerr << "No problem" << std::endl << std::endl;
  }
}

} // namespace

int main() {
  std::vector<std::string> keys_buffer;
  make_keys(keys_buffer);

  std::vector<std::string> others_buffer;
  make_other_keys(keys_buffer, others_buffer);

  std::vector<std::pair<const uint8_t*, size_t>> keys(keys_buffer.size());
  for (size_t i = 0; i < keys.size(); ++i) {
    keys[i] = {reinterpret_cast<const uint8_t*>(keys_buffer[i].c_str()),
               keys_buffer[i].length()};
  }

  std::vector<std::pair<const uint8_t*, size_t>> others(others_buffer.size());
  for (size_t i = 0; i < others.size(); ++i) {
    others[i] = {reinterpret_cast<const uint8_t*>(others_buffer[i].c_str()),
                 others_buffer[i].length()};
  }

  test_trie<false>(keys, others);
  test_trie<true>(keys, others);

  return 0;
}
