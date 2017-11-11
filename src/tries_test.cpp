#undef NDEBUG

#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <cstring>

#include "xcdat.hpp"

using namespace xcdat;

namespace {

constexpr size_t kNumStrings = 1U << 10;
constexpr size_t kMaxLength = 20;

using Key = TrieBuilder::Key;

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

std::vector<std::string> make_keys() {
  std::vector<std::string> keys;
  keys.reserve(kNumStrings);

  for (size_t i = 0; i < kNumStrings; ++i) {
    keys.push_back(make_key());
  }

  to_set(keys);
  return keys;
}

std::vector<std::string> make_other_keys(const std::vector<std::string>& keys) {
  std::vector<std::string> others;

  for (size_t i = 0; i < kNumStrings; ++i) {
    auto string = make_key();
    if (std::find(std::begin(keys), std::end(keys), string) == std::end(keys)) {
      others.push_back(string);
    }
  }

  to_set(others);
  return others;
}

template<bool Fast>
void test_build(Trie<Fast>& trie, const std::vector<Key>& keys, bool binary_mode) {
  std::cerr << "Construction -> build()" << std::endl;

  trie = TrieBuilder::build<Fast>(keys, binary_mode);
  assert(trie.num_keys() == keys.size());
}

template<bool Fast>
void test_basic_operations(const Trie<Fast>& trie, const std::vector<Key>& keys,
                           const std::vector<Key>& others) {
  std::cerr << "Basic operations -> lookup() and access()" << std::endl;

  for (auto& key : keys) {
    const auto id = trie.lookup(key.ptr, key.length);
    assert(id != kNotFound);

    std::vector<uint8_t> ret;
    trie.access(id, ret);

    assert(ret.size() == key.length);
    assert(std::memcmp(ret.data(), key.ptr, key.length) == 0);
  }

  for (auto& other : others) {
    const auto id = trie.lookup(other.ptr, other.length);
    assert(id == kNotFound);
  }
}

template<bool Fast>
void test_prefix_operations(const Trie<Fast>& trie, const std::vector<Key>& keys,
                            const std::vector<Key>& others) {
  std::cerr << "Prefix operations -> common_prefix_lookup()" << std::endl;

  for (auto& key : keys) {
    size_t num_results = 0;

    auto it = trie.make_prefix_iterator(key.ptr, key.length);
    while (it.next()) {
      auto id = it.id();
      auto dec = it.key();

      assert(dec.second <= key.length);

      std::vector<uint8_t> dec2;
      trie.access(id, dec2);

      assert(dec.second == dec2.size());
      assert(std::memcmp(dec.first, dec2.data(), dec.second) == 0);

      ++num_results;
    }

    assert(1 <= num_results);
    assert(num_results <= key.length);
  }

  for (auto& other : others) {
    size_t num_results = 0;

    auto it = trie.make_prefix_iterator(other.ptr, other.length);
    while (it.next()) {
      auto id = it.id();
      auto dec = it.key();

      assert(dec.second < other.length);

      std::vector<uint8_t> dec2;
      trie.access(id, dec2);

      assert(dec.second == dec2.size());
      assert(std::memcmp(dec.first, dec2.data(), dec.second) == 0);

      ++num_results;
    }

    assert(num_results < other.length);
  }
}

template<bool Fast>
void test_predictive_operations(const Trie<Fast>& trie, const std::vector<Key>& keys,
                                const std::vector<Key>& others) {
  std::cerr << "Predictive operations -> predictive_lookup()" << std::endl;

  for (auto& key : keys) {
    size_t num_results = 0;

    auto it = trie.make_predictive_iterator(key.ptr, key.length);
    while (it.next()) {
      auto id = it.id();
      auto dec = it.key();

      assert(key.length <= dec.second);

      std::vector<uint8_t> dec2;
      trie.access(id, dec2);

      assert(dec.second == dec2.size());
      assert(std::memcmp(dec.first, dec2.data(), dec.second) == 0);

      ++num_results;
    }

    assert(1 <= num_results);
  }

  for (auto& other : others) {
    auto it = trie.make_predictive_iterator(other.ptr, other.length);
    while (it.next()) {
      auto id = it.id();
      auto dec = it.key();

      assert(other.length < dec.second);

      std::vector<uint8_t> dec2;
      trie.access(id, dec2);

      assert(dec.second == dec2.size());
      assert(std::memcmp(dec.first, dec2.data(), dec.second) == 0);
    }
  }

  { // all enumeration
    size_t num_results = 0;

    auto it = trie.make_predictive_iterator(nullptr, 0);
    while (it.next()) {
      auto id = it.id();
      auto dec = it.key();

      assert(0 <= dec.second);

      std::vector<uint8_t> dec2;
      trie.access(id, dec2);

      assert(dec.second == dec2.size());
      assert(std::memcmp(dec.first, dec2.data(), dec.second) == 0);

      ++num_results;
    }

    assert(num_results == trie.num_keys());
  }
}

template<bool Fast>
void test_io(const Trie<Fast>& trie) {
  std::cerr << "File I/O -> write() and read()" << std::endl;

  const char* file_name = "index";
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
    _trie = Trie<Fast>(ifs);
  }

  assert(trie.num_keys() == _trie.num_keys());
  assert(trie.max_length() == _trie.max_length());
  assert(trie.is_binary_mode() == _trie.is_binary_mode());
  assert(trie.alphabet_size() == _trie.alphabet_size());
  assert(trie.num_nodes() == _trie.num_nodes());
  assert(trie.num_used_nodes() == _trie.num_used_nodes());
  assert(trie.num_free_nodes() == _trie.num_free_nodes());
  assert(trie.size_in_bytes() == _trie.size_in_bytes());
}

template<bool Fast>
void test_trie(const std::vector<Key>& strings, const std::vector<Key>& others) {
  for (int i = 0; i < 2; ++i) {
    std::cerr << "** " << (i % 2 ? "Binary" : "Text") << " Mode **" << std::endl;
    std::cerr << "Testing xcdat::Trie<" << (Fast ? "true" : "false") << ">" << std::endl;
    Trie<Fast> trie;
    test_build(trie, strings, i % 2 != 0);
    test_basic_operations(trie, strings, others);
    test_prefix_operations(trie, strings, others);
    test_predictive_operations(trie, strings, others);
    test_io(trie);
    std::cerr << "No problem" << std::endl << std::endl;
  }
}

} // namespace

int main() {
  auto keys_buffer = make_keys();
  auto others_buffer = make_other_keys(keys_buffer);

  std::vector<Key> keys(keys_buffer.size());
  for (size_t i = 0; i < keys.size(); ++i) {
    keys[i] = {reinterpret_cast<const uint8_t*>(keys_buffer[i].c_str()),
               keys_buffer[i].length()};
  }

  std::vector<Key> others(others_buffer.size());
  for (size_t i = 0; i < others.size(); ++i) {
    others[i] = {reinterpret_cast<const uint8_t*>(others_buffer[i].c_str()),
                 others_buffer[i].length()};
  }

  test_trie<false>(keys, others);
  test_trie<true>(keys, others);

  return 0;
}