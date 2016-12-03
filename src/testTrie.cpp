#undef NDEBUG

#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>

#include "Trie.hpp"

using namespace xcdat;

namespace {

constexpr size_t kNumStrings = 1U << 10;
constexpr size_t kMaxLength = 20;

void to_set(std::vector<std::string>& strings) {
  std::sort(std::begin(strings), std::end(strings));
  strings.erase(std::unique(std::begin(strings), std::end(strings)), std::end(strings));
}

std::string make_string() {
  std::random_device rnd;

  std::string str;
  size_t length = (rnd() % kMaxLength) + 1;
  for (size_t j = 0; j < length; ++j) {
    str += 'A' + (rnd() % 26);
  }

  return str;
}

void make_strings(std::vector<std::string>& strings) {
  strings.clear();
  strings.reserve(kNumStrings);

  for (size_t i = 0; i < kNumStrings; ++i) {
    strings.push_back(make_string());
  }

  to_set(strings);
}

void make_other_strings(const std::vector<std::string>& strings, std::vector<std::string>& others) {
  others.clear();

  for (size_t i = 0; i < kNumStrings; ++i) {
    auto string = make_string();
    if (std::find(std::begin(strings), std::end(strings), string) == std::end(strings)) {
      others.push_back(string);
    }
  }

  to_set(others);
}

template <bool Fast>
void test_build(Trie<Fast>& trie, const std::vector<CharRange>& strings) {
  std::cerr << "Construction -> build()" << std::endl;

  Trie<Fast>{strings}.swap(trie);
  assert(trie.num_strings() == strings.size());
}

template <bool Fast>
void test_basic_operations(const Trie<Fast>& trie, const std::vector<CharRange>& strings,
                           const std::vector<CharRange>& others) {
  std::cerr << "Basic operations -> lookup() and access()" << std::endl;

  for (size_t i = 0; i < strings.size(); ++i) {
    const auto id = trie.lookup(strings[i]);
    assert(id != kNotFound);
    assert(CharRange{trie.access(id)} == strings[i]);
  }

  for (size_t i = 0; i < others.size(); ++i) {
    const auto id = trie.lookup(others[i]);
    assert(id == kNotFound);
  }
}

template <bool Fast>
void test_prefix_operations(const Trie<Fast>& trie, const std::vector<CharRange>& strings,
                            const std::vector<CharRange>& others) {
  std::cerr << "Prefix operations -> common_prefix_lookup()" << std::endl;

  for (auto& string : strings) {
    std::vector<uint32_t> ids;
    auto num_ids = trie.common_prefix_lookup(string, ids);

    assert(1 <= num_ids);
    assert(num_ids <= kMaxLength);
    assert(num_ids == ids.size());

    for (auto id : ids) {
      assert(trie.access(id).length() <= string.length());
    }
  }

  for (auto& other : others) {
    std::vector<uint32_t> ids;
    auto num_ids = trie.common_prefix_lookup(other, ids);

    assert(num_ids <= kMaxLength);
    assert(num_ids == ids.size());

    for (auto id : ids) {
      assert(trie.access(id).length() < other.length());
    }
  }
}

template <bool Fast>
void test_predictive_operations(const Trie<Fast>& trie, const std::vector<CharRange>& strings,
                                const std::vector<CharRange>& others) {
  std::cerr << "Predictive operations -> predictive_lookup()" << std::endl;

  for (auto& string : strings) {
    std::vector<uint32_t> ids;
    auto num_ids = trie.predictive_lookup(string, ids);

    assert(1 <= num_ids);
    assert(num_ids == ids.size());

    for (auto id : ids) {
      assert(string.length() <= trie.access(id).length());
    }
  }

  for (auto& other : others) {
    std::vector<uint32_t> ids;
    auto num_ids = trie.predictive_lookup(other, ids);

    assert(num_ids == ids.size());

    for (auto id : ids) {
      assert(other.length() < trie.access(id).length());
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

  assert(trie.num_strings() == _trie.num_strings());
  assert(trie.alphabet_size() == _trie.alphabet_size());
  assert(trie.num_nodes() == _trie.num_nodes());
  assert(trie.num_used_nodes() == _trie.num_used_nodes());
  assert(trie.num_free_nodes() == _trie.num_free_nodes());
  assert(trie.size_in_bytes() == _trie.size_in_bytes());
}

template <bool Fast>
void test_trie(const std::vector<CharRange>& strings, const std::vector<CharRange>& others) {
  Trie<Fast> trie;

  std::cerr << "Testing xcdat::Trie<" << (Fast ? "true" : "false") << ">" << std::endl;
  test_build(trie, strings);
  test_basic_operations(trie, strings, others);
  test_prefix_operations(trie, strings, others);
  test_predictive_operations(trie, strings, others);
  test_io(trie);
  std::cerr << "OK!" << std::endl;
}

} // namespace

int main() {
  std::vector<std::string> strings_buffer;
  make_strings(strings_buffer);

  std::vector<std::string> others_buffer;
  make_other_strings(strings_buffer, others_buffer);

  std::vector<CharRange> strings(strings_buffer.size());
  for (size_t i = 0; i < strings.size(); ++i) {
    strings[i] = {strings_buffer[i]};
  }

  std::vector<CharRange> others(others_buffer.size());
  for (size_t i = 0; i < others.size(); ++i) {
    others[i] = {others_buffer[i]};
  }

  test_trie<false>(strings, others);
  test_trie<true>(strings, others);

  return 0;
}
