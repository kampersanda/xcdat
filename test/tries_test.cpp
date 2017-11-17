#undef NDEBUG

#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <cstring>

#include "xcdat.hpp"

using namespace xcdat;

namespace {

constexpr size_t NUM_KEYS = 1U << 10;
constexpr size_t MAX_LENGTH = 20;

void to_set(std::vector<std::string>& keys) {
  std::sort(std::begin(keys), std::end(keys));
  keys.erase(std::unique(std::begin(keys), std::end(keys)), std::end(keys));
}

std::string make_key() {
  std::random_device rnd;

  std::string key;
  size_t length = (rnd() % MAX_LENGTH) + 1;
  for (size_t j = 0; j < length; ++j) {
    key += 'A' + (rnd() % 26);
  }

  return key;
}

std::vector<std::string> make_keys() {
  std::vector<std::string> keys;
  keys.reserve(NUM_KEYS);

  for (size_t i = 0; i < NUM_KEYS; ++i) {
    keys.push_back(make_key());
  }

  to_set(keys);
  return keys;
}

std::vector<std::string> make_other_keys(const std::vector<std::string>& keys) {
  std::vector<std::string> others;

  for (size_t i = 0; i < NUM_KEYS; ++i) {
    auto string = make_key();
    if (std::find(std::begin(keys), std::end(keys), string) == std::end(keys)) {
      others.push_back(string);
    }
  }

  to_set(others);
  return others;
}

template<bool Fast>
Trie<Fast> test_build(const std::vector<std::string_view>& keys,
                      bool bin_mode) {
  std::cerr << "Construction -> build()\n";

  auto trie = TrieBuilder::build<Fast>(keys, bin_mode);
  assert(trie.num_keys() == keys.size());

  return trie;
}

template<bool Fast>
void test_basic_operations(const Trie<Fast>& trie,
                           const std::vector<std::string_view>& keys,
                           const std::vector<std::string_view>& others) {
  std::cerr << "Basic operations -> lookup() and access()\n";

  for (auto& key : keys) {
    auto id = trie.lookup(key);
    assert(id != Trie<Fast>::NOT_FOUND);

    auto dec = trie.access(id);
    assert(dec == key);
  }

  for (auto& other : others) {
    const auto id = trie.lookup(other);
    assert(id == Trie<Fast>::NOT_FOUND);
  }
}

template<bool Fast>
void test_prefix_operations(const Trie<Fast>& trie,
                            const std::vector<std::string_view>& keys,
                            const std::vector<std::string_view>& others) {
  std::cerr << "Prefix operations -> PrefixIterator\n";

  for (auto& key : keys) {
    size_t num_results = 0;

    auto it = trie.make_prefix_iterator(key);
    while (it.next()) {
      auto id = it.id();
      auto dec = it.key();

      assert(dec.length() <= key.length());

      auto dec2 = trie.access(id);
      assert(dec == dec2);

      ++num_results;
    }

    assert(1 <= num_results);
    assert(num_results <= key.length());
  }

  for (auto& other : others) {
    size_t num_results = 0;

    auto it = trie.make_prefix_iterator(other);
    while (it.next()) {
      auto id = it.id();
      auto dec = it.key();

      assert(dec.length() < other.length());

      auto dec2 = trie.access(id);
      assert(dec == dec2);

      ++num_results;
    }

    assert(num_results < other.length());
  }
}

template<bool Fast>
void test_predictive_operations(const Trie<Fast>& trie,
                                const std::vector<std::string_view>& keys,
                                const std::vector<std::string_view>& others) {
  std::cerr << "Predictive operations -> PredictiveIterator\n";

  for (auto& key : keys) {
    size_t num_results = 0;

    auto it = trie.make_predictive_iterator(key);
    while (it.next()) {
      auto id = it.id();
      auto dec = it.key();

      assert(key.length() <= dec.length());

      auto dec2 = trie.access(id);
      assert(dec == dec2);

      ++num_results;
    }

    assert(1 <= num_results);
  }

  for (auto& other : others) {
    auto it = trie.make_predictive_iterator(other);
    while (it.next()) {
      auto id = it.id();
      auto dec = it.key();

      assert(other.length() < dec.length());

      auto dec2 = trie.access(id);

      assert(dec == dec2);
    }
  }

  { // all enumeration
    size_t num_results = 0;

    auto it = trie.make_predictive_iterator(std::string_view{});
    while (it.next()) {
      auto id = it.id();
      auto dec = it.key();

      assert(0 <= dec.length());

      auto dec2 = trie.access(id);
      assert(dec == dec2);

      ++num_results;
    }

    assert(num_results == trie.num_keys());
  }
}

template<bool Fast>
void test_io(const Trie<Fast>& trie) {
  std::cerr << "File I/O -> write() and read()\n";

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
  assert(trie.bin_mode() == _trie.bin_mode());
  assert(trie.alphabet_size() == _trie.alphabet_size());
  assert(trie.num_nodes() == _trie.num_nodes());
  assert(trie.num_used_nodes() == _trie.num_used_nodes());
  assert(trie.num_free_nodes() == _trie.num_free_nodes());
  assert(trie.size_in_bytes() == _trie.size_in_bytes());
}

template<bool Fast>
void test_trie(const std::vector<std::string_view>& strings,
               const std::vector<std::string_view>& others) {
  for (int i = 0; i < 2; ++i) {
    std::cerr << "** " << (i % 2 ? "Binary" : "Text") << " Mode **\n";
    std::cerr << "Testing xcdat::Trie<" << (Fast ? "true" : "false") << ">\n";

    auto trie = test_build<Fast>(strings, i % 2 != 0);

    test_basic_operations(trie, strings, others);
    test_prefix_operations(trie, strings, others);
    test_predictive_operations(trie, strings, others);
    test_io(trie);

    std::cerr << "--> No problem (☝ ՞ਊ ՞)☝" << std::endl << std::endl;
  }
}

} // namespace

int main() {
  auto keys_buffer = make_keys();
  auto others_buffer = make_other_keys(keys_buffer);

  std::vector<std::string_view> keys(keys_buffer.size());
  for (size_t i = 0; i < keys.size(); ++i) {
    keys[i] = std::string_view{keys_buffer[i]};
  }

  std::vector<std::string_view> others(others_buffer.size());
  for (size_t i = 0; i < others.size(); ++i) {
    others[i] = std::string_view{others_buffer[i]};
  }

  test_trie<false>(keys, others);
  test_trie<true>(keys, others);

  return 0;
}
