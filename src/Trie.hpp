#ifndef XCDAT_TRIE_HPP_
#define XCDAT_TRIE_HPP_

#include "DacBc.hpp"
#include "FastDacBc.hpp"
#include "TrieBuilder.hpp"

namespace xcdat {

constexpr auto kNotFound = static_cast<uint32_t>(-1);
constexpr auto kDefaultLimit = static_cast<size_t>(-1);

/*
 * A static compressed string dictionary based on an improved double-array trie.
 *  @param Fast
 * */
template<bool Fast>
class Trie {
public:
  using Type = Trie<Fast>;
  using BcType = typename std::conditional<Fast, FastDacBc, DacBc>::type;

  Trie() {}

  /*
   * Builds a trie dictionary from a set of strings in lexicographical order.
   * */
  Trie(const std::vector<CharRange>& strings);

  ~Trie() {}

  // Returns the string ID if stored, otherwise 'kNotFound'.
  uint32_t lookup(CharRange string) const;

  // Returns the corresponding string, access(lookup())
  std::string access(uint32_t id) const;

  // Return the corresponding string.
  size_t common_prefix_lookup(CharRange string, std::vector<uint32_t>& ids,
                              size_t limit = kDefaultLimit) const;

  // Return the corresponding string.
  size_t predictive_lookup(CharRange string, std::vector<uint32_t>& ids,
                           size_t limit = kDefaultLimit) const;

  size_t num_strings() const {
    return num_strings_;
  }
  size_t alphabet_size() const {
    return alphabet_.size();
  }

  size_t num_nodes() const {
    return bc_.num_nodes();
  }
  size_t num_used_nodes() const {
    return bc_.num_used_nodes();
  }
  size_t num_free_nodes() const {
    return bc_.num_free_nodes();
  }

  // Returns the number of bytes.
  size_t size_in_bytes() const;

  // Dumps statistics of the dictionary.
  void show_stat(std::ostream& os) const;

  void write(std::ostream& os) const;

  void read(std::istream& is);

  void swap(Type& rhs);

  Trie(const Trie&) = delete;
  Trie& operator=(const Trie&) = delete;

private:
  BcType bc_;
  BitVector terms_;
  std::vector<uint8_t> tail_;
  std::vector<uint8_t> alphabet_;
  std::array<uint8_t, 512> table_; // table[table[c] + 256] = c

  size_t num_strings_ = 0;
  size_t max_length_ = 0;

  uint32_t to_string_id_(uint32_t node_id) const {
    return terms_.rank(node_id);
  };
  uint32_t to_node_id_(uint32_t string_id) const {
    return terms_.select(string_id);
  };
  uint8_t edge_(uint32_t node_id, uint32_t child_id) const {
    return table_[static_cast<uint8_t>(bc_.base(node_id) ^ child_id) + 256];
  }

  bool match_(CharRange string, uint32_t link) const;
  bool prefix_match_(CharRange string, uint32_t link) const;
  void enumerate_ids_(uint32_t node_id, std::vector<uint32_t>& ids,
                      size_t& num_ids, size_t limit) const;
};

/*
 * Member functions
 * */

template<bool Fast>
Trie<Fast>::Trie(const std::vector<CharRange>& strings) {
  TrieBuilder builder{strings, BcType::kFirstBits};

  BcType{builder.bc_}.swap(bc_);
  BitVector{builder.terms_, true}.swap(terms_);
  tail_ = std::move(builder.tail_);
  alphabet_ = std::move(builder.alphabet_);
  table_ = builder.table_;

  num_strings_ = strings.size();
  max_length_ = builder.max_length_;
}

template<bool Fast>
uint32_t Trie<Fast>::lookup(CharRange string) const {
  uint32_t node_id = 0;

  while (!bc_.is_leaf(node_id)) {
    if (string.begin == string.end) {
      return terms_[node_id] ? to_string_id_(node_id) : kNotFound;
    }

    const auto child_id = bc_.base(node_id) ^ table_[*string.begin++];

    if (bc_.check(child_id) != node_id) {
      return kNotFound;
    }

    node_id = child_id;
  }

  if (match_(string, bc_.link(node_id))) {
    return to_string_id_(node_id);
  }
  return kNotFound;
}

template<bool Fast>
std::string Trie<Fast>::access(uint32_t id) const {
  if (num_strings_ <= id) {
    return {};
  }

  std::string ret;
  ret.reserve(max_length_);

  auto node_id = to_node_id_(id);
  const auto link = bc_.is_leaf(node_id) ? bc_.link(node_id) : kNotFound;

  while (node_id) {
    const auto parent_id = bc_.check(node_id);
    ret += edge_(parent_id, node_id);
    node_id = parent_id;
  }

  std::reverse(std::begin(ret), std::end(ret));
  if (link != kNotFound) {
    ret += reinterpret_cast<const char*>(tail_.data()) + link;
  }

  return ret; // expecting move semantics
}

template<bool Fast>
size_t Trie<Fast>::common_prefix_lookup(CharRange string, std::vector<uint32_t>& ids,
                                        size_t limit) const {
  if (limit == 0) {
    return 0;
  }

  uint32_t node_id = 0;
  size_t num_ids = 0;

  while (!bc_.is_leaf(node_id)) {
    if (terms_[node_id]) {
      ids.push_back(to_string_id_(node_id));
      ++num_ids;
      if (num_ids == limit) {
        return num_ids;
      }
    }
    if (string.begin == string.end) {
      return num_ids;
    }

    const auto child_id = bc_.base(node_id) ^ table_[*string.begin++];

    if (bc_.check(child_id) != node_id) {
      return num_ids;
    }

    node_id = child_id;
  }

  if (match_(string, bc_.link(node_id))) {
    ids.push_back(to_string_id_(node_id));
    ++num_ids;
  }

  return num_ids;
}

template<bool Fast>
size_t Trie<Fast>::predictive_lookup(CharRange string, std::vector<uint32_t>& ids,
                                     size_t limit) const {
  uint32_t node_id = 0;

  for (; string.begin != string.end; ++string.begin) {
    if (bc_.is_leaf(node_id)) {
      if (prefix_match_(string, bc_.link(node_id))) {
        ids.push_back(to_string_id_(node_id));
        return 1;
      }
      return 0;
    }

    const auto child_id = bc_.base(node_id) ^ table_[*string.begin];

    if (bc_.check(child_id) != node_id) {
      return 0;
    }
    node_id = child_id;
  }

  size_t num_ids = 0;
  enumerate_ids_(node_id, ids, num_ids, limit);
  return num_ids;
}

template<bool Fast>
size_t Trie<Fast>::size_in_bytes() const {
  size_t ret = 0;
  ret += bc_.size_in_bytes();
  ret += terms_.size_in_bytes();
  ret += size_vector(tail_);
  ret += size_vector(alphabet_);
  ret += sizeof(table_);
  ret += sizeof(num_strings_);
  ret += sizeof(max_length_);
  return ret;
}

template<bool Fast>
void Trie<Fast>::show_stat(std::ostream& os) const {
  const auto total_size = size_in_bytes();
  os << "basic statistics of xcdat::Trie" << std::endl;
  show_size("\tnum strings:   ", num_strings(), os);
  show_size("\talphabet size: ", alphabet_size(), os);
  show_size("\tnum nodes:     ", num_nodes(), os);
  show_size("\tnum used_nodes:", num_used_nodes(), os);
  show_size("\tnum free_nodes:", num_free_nodes(), os);
  show_size("\tsize in bytes: ", size_in_bytes(), os);
  os << "member size statistics of xcdat::Trie" << std::endl;
  show_size_ratio("\tbc_:      ", bc_.size_in_bytes(), total_size, os);
  show_size_ratio("\tterms_:   ", terms_.size_in_bytes(), total_size, os);
  show_size_ratio("\ttail_:    ", size_vector(tail_), total_size, os);
  show_size_ratio("\talphabet_:", size_vector(alphabet_), total_size, os);
  show_size_ratio("\ttable_:   ", sizeof(table_), total_size, os);
  bc_.show_stat(os);
}

template<bool Fast>
void Trie<Fast>::write(std::ostream& os) const {
  bc_.write(os);
  terms_.write(os);
  write_vector(tail_, os);
  write_vector(alphabet_, os);
  write_value(table_, os);
  write_value(num_strings_, os);
  write_value(max_length_, os);
}

template<bool Fast>
void Trie<Fast>::read(std::istream& is) {
  bc_.read(is);
  terms_.read(is);
  read_vector(tail_, is);
  read_vector(alphabet_, is);
  read_value(table_, is);
  read_value(num_strings_, is);
  read_value(max_length_, is);
}

template<bool Fast>
void Trie<Fast>::swap(Type& rhs) {
  bc_.swap(rhs.bc_);
  terms_.swap(rhs.terms_);
  tail_.swap(rhs.tail_);
  alphabet_.swap(rhs.alphabet_);
  table_.swap(rhs.table_);
  std::swap(num_strings_, rhs.num_strings_);
  std::swap(max_length_, rhs.max_length_);
}

template<bool Fast>
bool Trie<Fast>::match_(CharRange string, uint32_t link) const {
  auto tail = tail_.data() + link;
  for (auto it = string.begin; it != string.end; ++it, ++tail) {
    if (*tail == '\0' || *it != *tail) {
      return false;
    }
  }
  return *tail == '\0';
}

template<bool Fast>
bool Trie<Fast>::prefix_match_(CharRange string, uint32_t link) const {
  auto tail = tail_.data() + link;
  for (auto it = string.begin; it != string.end; ++it, ++tail) {
    if (*tail == '\0' || *it != *tail) {
      return false;
    }
  }
  return true;
}

template<bool Fast>
void Trie<Fast>::enumerate_ids_(uint32_t node_id, std::vector<uint32_t>& ids, size_t& num_ids,
                                size_t limit) const {
  if (terms_[node_id]) {
    ids.push_back(to_string_id_(node_id));
    ++num_ids;
    if (bc_.is_leaf(node_id)) {
      return;
    }
  }
  const auto base = bc_.base(node_id);
  for (const auto label : alphabet_) {
    if (num_ids == limit) {
      break;
    }
    const auto child_id = base ^ table_[label];
    if (bc_.check(child_id) == node_id) {
      enumerate_ids_(child_id, ids, num_ids, limit);
    }
  }
}

} //namespace - xcdat

#endif //XCDAT_TRIE_HPP_
