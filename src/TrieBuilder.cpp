#include "TrieBuilder.hpp"

namespace xcdat {

#define XCDAT_THROW(message) \
  throw Exception(message, __FILE__, __func__, __LINE__)

TrieBuilder::TrieBuilder(const std::vector<CharRange>& strings,
                         uint32_t first_bit_size)
  : strings_(strings), block_size_(1U << first_bit_size) {
  if (strings_.empty() || kBcUpper < strings_.size()) {
    XCDAT_THROW("The number of strings is out of range.");
  }

  {
    size_t init_capacity = 1;
    while (init_capacity < strings_.size()) {
      init_capacity <<= 1;
    }
    bc_.reserve(init_capacity);
    terms_.reserve(init_capacity);
    heads_.reserve(init_capacity / block_size_);
  }

  alphabet_.reserve(256);
  edges_.reserve(256);
  suffixes_.reserve(strings_.size());

  for (uint32_t i = 0; i < 256; ++i) {
    bc_.push_back({i + 1, false, i - 1, false});
    terms_.push_back(false);
  }
  bc_[255].base = 0;
  bc_[0].check = 255;

  for (uint32_t i = 0; i < 256; i += block_size_) {
    heads_.emplace_back(i);
  }

  use_(0);
  bc_[0].check = kTabooId;
  bc_[kTabooId].is_used = true;
  heads_[kTabooId / block_size_] = bc_[kTabooId].base;

  build_table_();
  build_bc_(0, strings_.size(), 0, 0);
  build_tail_();
}

void TrieBuilder::build_table_() {
  std::array<std::pair<uint8_t, size_t>, 256> table_builder;

  for (uint32_t i = 0; i < 256; ++i) {
    table_builder[i] = {static_cast<uint8_t>(i), 0};
  }

  auto char_count = [&](const CharRange& string) {
    for (auto it = string.begin; it != string.end; ++it) {
      ++table_builder[*it].second;
    }
  };

  char_count(strings_[0]);
  max_length_ = strings_[0].length();

  for (size_t i = 1; i < strings_.size(); ++i) {
    if (!(strings_[i - 1] < strings_[i])) {
      XCDAT_THROW("The input strings do not consist of a set in lexicographical order.");
    }
    char_count(strings_[i]);
    max_length_ = std::max(max_length_, strings_[i].length());
  }

  if (table_builder[0].second) {
    XCDAT_THROW("The input strings include an ASCII zero character.");
  }

  for (const auto& item : table_builder) {
    if (item.second != 0) {
      alphabet_.push_back(item.first);
    }
  }
  alphabet_.shrink_to_fit();

  std::sort(std::begin(table_builder), std::end(table_builder),
            [](const std::pair<uint8_t, size_t>& lhs, const std::pair<uint8_t, size_t>& rhs) {
              return lhs.second > rhs.second;
            });

  for (uint32_t i = 0; i < 256; ++i) {
    table_[table_builder[i].first] = static_cast<uint8_t>(i);
  }

  for (uint32_t i = 0; i < 256; ++i) {
    table_[table_[i] + 256] = static_cast<uint8_t>(i);
  }
}

void TrieBuilder::build_bc_(size_t begin, size_t end, size_t depth, uint32_t node_id) {
  if (strings_[begin].length() == depth) {
    terms_.set_bit(node_id, true);
    if (++begin == end) { // without link
      bc_[node_id].base = 0;
      bc_[node_id].is_leaf = true;
      return;
    }
  } else if (begin + 1 == end) { // leaf
    terms_.set_bit(node_id, true);
    auto& string = strings_[begin];
    suffixes_.push_back({{string.begin + depth, string.end}, node_id});
    return;
  }

  { // Fetching edges
    edges_.clear();
    auto label = strings_[begin].begin[depth];
    for (auto str_id = begin + 1; str_id < end; ++str_id) {
      const auto _label = strings_[str_id].begin[depth];
      if (label != _label) {
        edges_.push_back(label);
        label = _label;
      }
    }
    edges_.push_back(label);
  }

  const auto base = find_base_(node_id / block_size_);
  if (bc_.size() <= base) {
    expand_();
  }

  // Defining new edges
  bc_[node_id].base = base;
  for (const auto label : edges_) {
    const auto child_id = base ^ table_[label];
    use_(child_id);
    bc_[child_id].check = node_id;
  }

  // Following children
  auto _begin = begin;
  auto label = strings_[begin].begin[depth];
  for (auto _end = begin + 1; _end < end; ++_end) {
    const auto _label = strings_[_end].begin[depth];
    if (label != _label) {
      build_bc_(_begin, _end, depth + 1, base ^ table_[label]);
      label = _label;
      _begin = _end;
    }
  }
  build_bc_(_begin, end, depth + 1, base ^ table_[label]);
}

void TrieBuilder::build_tail_() {
  auto can_unify = [](const Suffix& lhs, const Suffix& rhs) {
    if (lhs.string.length() > rhs.string.length()) {
      return false;
    }

    auto lhs_range = std::make_pair(lhs.rbegin(), lhs.rend());
    auto rhs_range = std::make_pair(rhs.rbegin(), rhs.rend());

    while (lhs_range.first != lhs_range.second) {
      if (*lhs_range.first != *rhs_range.first) {
        return false;
      }
      ++lhs_range.first;
      ++rhs_range.first;
    }
    return true;
  };

  std::sort(std::begin(suffixes_), std::end(suffixes_),
            [](const Suffix& lhs, const Suffix& rhs) {
              return std::lexicographical_compare(
                lhs.rbegin(), lhs.rend(), rhs.rbegin(), rhs.rend()
              );
            });

  tail_.push_back('\0'); // for an empty suffix

  size_t begin = 0;
  for (size_t i = 1; i < suffixes_.size(); ++i) {
    const auto& lhs = suffixes_[i - 1];
    const auto& rhs = suffixes_[i];

    if (can_unify(lhs, rhs)) {
      continue;
    }

    append_tail_(begin, i, lhs.string);
    begin = i;
  }

  append_tail_(begin, suffixes_.size(), suffixes_.back().string);
  tail_.shrink_to_fit();
}

void TrieBuilder::expand_() {
  if (kBcUpper < bc_.size() + 256) {
    XCDAT_THROW("The length of BASE/CHECK is out of range.");
  }

  const auto old_size = static_cast<uint32_t>(bc_.size());
  const auto new_size = old_size + 256;

  for (auto i = old_size; i < new_size; ++i) {
    bc_.push_back({i + 1, false, i - 1, false});
    terms_.push_back(false);
  }

  {
    const auto last = bc_[kTabooId].check;
    bc_[old_size].check = last;
    bc_[last].base = old_size;
    bc_[new_size - 1].base = kTabooId;
    bc_[kTabooId].check = new_size - 1;
  }

  for (auto i = old_size; i < new_size; i += block_size_) {
    heads_.push_back(i);
  }

  const auto block_id = old_size / 256;
  if (kFreeBlocks <= block_id) {
    close_block_(block_id - kFreeBlocks);
  }
}

void TrieBuilder::use_(uint32_t node_id) {
  bc_[node_id].is_used = true;

  const auto next = bc_[node_id].base;
  const auto prev = bc_[node_id].check;
  bc_[prev].base = next;
  bc_[next].check = prev;

  const auto block_id = node_id / block_size_;
  if (heads_[block_id] == node_id) {
    heads_[block_id] = (block_id != next / block_size_) ? kTabooId : next;
  }
}

void TrieBuilder::close_block_(uint32_t block_id) {
  const auto begin = block_id * 256;
  const auto end = begin + 256;

  for (auto i = begin; i < end; ++i) {
    if (!bc_[i].is_used) {
      use_(i);
      bc_[i].is_used = false;
    }
  }

  for (auto i = begin; i < end; i += block_size_) {
    heads_[i / block_size_] = kTabooId;
  }
}

uint32_t TrieBuilder::find_base_(uint32_t block_id) const {
  if (bc_[kTabooId].base == kTabooId) { // Full?
    return static_cast<uint32_t>(bc_.size()) ^ table_[edges_[0]];
  }

  // search in the same block
  for (auto i = heads_[block_id]; i != kTabooId && i / block_size_ == block_id; i = bc_[i].base) {
    const auto base = i ^ table_[edges_[0]];
    if (is_target_(base)) {
      return base; // base / block_size_ == block_id
    }
  }

  for (auto i = bc_[kTabooId].base; i != kTabooId; i = bc_[i].base) {
    const auto base = i ^ table_[edges_[0]];
    if (is_target_(base)) {
      return base; // base / block_size_ != block_id
    }
  }

  return static_cast<uint32_t>(bc_.size()) ^ table_[edges_[0]];
}

bool TrieBuilder::is_target_(uint32_t base) const {
  for (const auto label : edges_) {
    const auto i = base ^ table_[label];
    if (bc_[i].is_used) {
      return false;
    }
  }
  return true;
}

void TrieBuilder::append_tail_(size_t begin, size_t end, const CharRange& string) {
  for (auto it = string.begin; it != string.end; ++it) {
    tail_.push_back(*it);
  }
  while (begin < end) {
    const auto& suffix = suffixes_[begin++];
    const auto tail_offset = tail_.size() - suffix.string.length();
    if (kBcUpper < tail_offset) {
      XCDAT_THROW("A pointer to TAIL is out of range.");
    }
    bc_[suffix.node_id].base = static_cast<uint32_t>(tail_offset);
    bc_[suffix.node_id].is_leaf = true;
  }
  tail_.push_back('\0'); // terminator
}

} //namespace - xcdat
