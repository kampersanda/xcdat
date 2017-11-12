#include <iostream>

#include "xcdat/TrieBuilder.hpp"

namespace xcdat {

TrieBuilder::TrieBuilder(const std::vector<std::string_view>& keys,
                         id_type width_L1, bool bin_mode)
  : keys_(keys), block_size_(1U << width_L1), width_L1_(width_L1),
    bin_mode_(bin_mode) {
  if (keys_.empty()) {
    throw TrieBuilder::Exception("The input data is empty.");
  }
  if (ID_MAX < keys_.size()) {
    throw TrieBuilder::Exception("Key ID range error.");
  }

  {
    size_t init_capa = 1;
    while (init_capa < keys_.size()) {
      init_capa <<= 1;
    }

    bc_.reserve(init_capa);
    leaf_flags_.reserve(init_capa);
    term_flags_.reserve(init_capa);
    used_flags_.reserve(init_capa);
    heads_.reserve(init_capa >> width_L1_);
  }

  alphabet_.reserve(256);
  edges_.reserve(256);
  suffixes_.reserve(keys_.size());

  // initialize an empty list.
  for (id_type i = 0; i < 256; ++i) {
    bc_.push_back({i + 1, i - 1});
    leaf_flags_.push_back(false);
    term_flags_.push_back(false);
    used_flags_.push_back(false);
  }
  bc_[255].base = 0;
  bc_[0].check = 255;

  for (id_type i = 0; i < 256; i += block_size_) {
    heads_.push_back(i);
  }

  use_(0);
  bc_[0].check = TABOO_ID;
  used_flags_[TABOO_ID] = true;
  heads_[TABOO_ID >> width_L1_] = bc_[TABOO_ID].base;

  build_table_();
  build_bc_(0, keys_.size(), 0, 0);
  build_tail_();
}

void TrieBuilder::build_table_() {
  using tb_type = std::pair<uint8_t, size_t>;
  tb_type table_builder[256];

  for (uint32_t i = 0; i < 256; ++i) {
    table_builder[i] = {static_cast<uint8_t>(i), 0};
  }

  max_length_ = 0;
  for (auto& key : keys_) {
    for (char c : key) {
      ++table_builder[static_cast<uint8_t>(c)].second;
    }
    max_length_ = std::max(max_length_, key.length());
  }

  if (table_builder[0].second != 0) { // including '\0'
    bin_mode_ = true;
  }

  for (const auto& item : table_builder) {
    if (item.second != 0) {
      alphabet_.push_back(item.first);
    }
  }
  alphabet_.shrink_to_fit();

  std::sort(std::begin(table_builder), std::end(table_builder),
            [](const tb_type& lhs, const tb_type& rhs) {
              return lhs.second > rhs.second;
            });

  for (uint32_t i = 0; i < 256; ++i) {
    table_[table_builder[i].first] = static_cast<uint8_t>(i);
  }

  for (uint32_t i = 0; i < 256; ++i) {
    table_[table_[i] + 256] = static_cast<uint8_t>(i);
  }
}

void TrieBuilder::build_bc_(size_t begin, size_t end, size_t depth,
                            id_type node_id) {
  if (keys_[begin].length() == depth) {
    term_flags_.set_bit(node_id, true);
    if (++begin == end) { // without link?
      bc_[node_id].base = 0; // with an empty suffix
      leaf_flags_.set_bit(node_id, true);
      return;
    }
  } else if (begin + 1 == end) { // leaf?
    term_flags_.set_bit(node_id, true);
    leaf_flags_.set_bit(node_id, true);
    auto& key = keys_[begin];
    suffixes_.push_back(
      {{key.data() + depth, key.length() - depth}, node_id}
    );
    return;
  }

  { // fetching edges
    edges_.clear();
    auto label = static_cast<uint8_t>(keys_[begin][depth]);
    for (auto str_id = begin + 1; str_id < end; ++str_id) {
      const auto _label = static_cast<uint8_t>(keys_[str_id][depth]);
      if (label != _label) {
        if (_label < label) {
          throw TrieBuilder::Exception(
            "The input data is not in lexicographical order."
          );
        }
        edges_.push_back(label);
        label = _label;
      }
    }
    edges_.push_back(label);
  }

  const auto base = find_base_(node_id >> width_L1_);
  if (bc_.size() <= base) {
    expand_();
  }

  // defining new edges
  bc_[node_id].base = base;
  for (const auto label : edges_) {
    const auto child_id = base ^ table_[label];
    use_(child_id);
    bc_[child_id].check = node_id;
  }

  // following the children
  auto _begin = begin;
  auto label = static_cast<uint8_t>(keys_[begin][depth]);
  for (auto _end = begin + 1; _end < end; ++_end) {
    const auto _label = static_cast<uint8_t>(keys_[_end][depth]);
    if (label != _label) {
      build_bc_(_begin, _end, depth + 1, base ^ table_[label]);
      label = _label;
      _begin = _end;
    }
  }
  build_bc_(_begin, end, depth + 1, base ^ table_[label]);
}

// The algorithm is inspired by marisa-trie
void TrieBuilder::build_tail_() {
  std::sort(std::begin(suffixes_), std::end(suffixes_),
            [](const Suffix& lhs, const Suffix& rhs) {
              return std::lexicographical_compare(
                std::rbegin(lhs), std::rend(lhs),
                std::rbegin(rhs), std::rend(rhs));
            });

  // For empty suffixes
  tail_.emplace_back('\0');
  if (bin_mode_) {
    boundary_flags_.push_back(false);
  }

  const Suffix dummy = {{nullptr, 0}, 0};
  const Suffix* prev_suf = &dummy;

  for (size_t i = suffixes_.size(); i > 0; --i) {
    const auto& cur_suf = suffixes_[i - 1];
    if (cur_suf.length() == 0) {
      throw TrieBuilder::Exception("A suffix is empty.");
    }

    size_t match = 0;
    while ((match < cur_suf.length()) && (match < prev_suf->length())
           && ((*prev_suf)[match] == cur_suf[match])) {
      ++match;
    }

    if ((match == cur_suf.length()) && (prev_suf->length() != 0)) { // sharing
      bc_[cur_suf.node_id].base = static_cast<id_type>(
        bc_[prev_suf->node_id].base + (prev_suf->length() - match)
      );
    } else { // append
      bc_[cur_suf.node_id].base = static_cast<id_type>(tail_.size());
      std::copy(std::begin(cur_suf.str), std::end(cur_suf.str),
                std::back_inserter(tail_));
      if (bin_mode_) {
        for (size_t j = 1; j < cur_suf.length(); ++j) {
          boundary_flags_.push_back(false);
        }
        boundary_flags_.push_back(true);
      } else {
        tail_.emplace_back('\0');
      }
      if (ID_MAX < tail_.size()) {
        throw TrieBuilder::Exception("TAIL address range error.");
      }
    }
    prev_suf = &cur_suf;
  }
}

void TrieBuilder::expand_() {
  if (ID_MAX < bc_.size() + 256) {
    throw TrieBuilder::Exception("Node ID range error.");
  }

  const auto old_size = static_cast<id_type>(bc_.size());
  const auto new_size = old_size + 256;

  for (auto i = old_size; i < new_size; ++i) {
    bc_.push_back({i + 1, i - 1});
    leaf_flags_.push_back(false);
    term_flags_.push_back(false);
    used_flags_.push_back(false);
  }

  {
    const auto last = bc_[TABOO_ID].check;
    bc_[old_size].check = last;
    bc_[last].base = old_size;
    bc_[new_size - 1].base = TABOO_ID;
    bc_[TABOO_ID].check = new_size - 1;
  }

  for (auto i = old_size; i < new_size; i += block_size_) {
    heads_.push_back(i);
  }

  const auto block_id = old_size / 256;
  if (FREE_BLOCKS <= block_id) {
    close_block_(block_id - FREE_BLOCKS);
  }
}

void TrieBuilder::use_(id_type node_id) {
  used_flags_[node_id] = true;

  const auto next = bc_[node_id].base;
  const auto prev = bc_[node_id].check;
  bc_[prev].base = next;
  bc_[next].check = prev;

  const auto block_id = node_id >> width_L1_;
  if (heads_[block_id] == node_id) {
    heads_[block_id] = (block_id != next >> width_L1_) ? TABOO_ID : next;
  }
}

void TrieBuilder::close_block_(id_type block_id) {
  const auto begin = block_id * 256;
  const auto end = begin + 256;

  for (auto i = begin; i < end; ++i) {
    if (!used_flags_[i]) {
      use_(i);
      bc_[i].base = i;
      bc_[i].check = i;
      used_flags_[i] = false;
    }
  }

  for (auto i = begin; i < end; i += block_size_) {
    heads_[i >> width_L1_] = TABOO_ID;
  }
}

id_type TrieBuilder::find_base_(id_type block_id) const {
  if (bc_[TABOO_ID].base == TABOO_ID) { // Full?
    return static_cast<id_type>(bc_.size()) ^ table_[edges_[0]];
  }

  // search in the same block
  for (auto i = heads_[block_id];
       i != TABOO_ID && i >> width_L1_ == block_id;
       i = bc_[i].base) {
    const auto base = i ^ table_[edges_[0]];
    if (is_target_(base)) {
      return base; // base / block_size_ == block_id
    }
  }

  for (auto i = bc_[TABOO_ID].base; i != TABOO_ID; i = bc_[i].base) {
    const auto base = i ^ table_[edges_[0]];
    if (is_target_(base)) {
      return base; // base / block_size_ != block_id
    }
  }

  return static_cast<id_type>(bc_.size()) ^ table_[edges_[0]];
}

bool TrieBuilder::is_target_(id_type base) const {
  for (const auto label : edges_) {
    if (used_flags_[base ^ table_[label]]) {
      return false;
    }
  }
  return true;
}

} //namespace - xcdat
