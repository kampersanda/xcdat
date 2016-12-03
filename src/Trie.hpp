#ifndef XCDAT_TRIE_HPP_
#define XCDAT_TRIE_HPP_

#include "TrieBuilder.hpp"
#include "DacBc.hpp"
#include "FastDacBc.hpp"

namespace xcdat {

// A static compressed string dictionary based on an improved double-array trie.
template<bool Fast>
class Trie {
public:
  using Type = Trie<Fast>;
  using BcType = typename std::conditional<Fast, FastDacBc, DacBc>::type;

  static constexpr auto kDefaultLimit = static_cast<size_t>(-1);

  Trie() {}

  // Builds a trie dictionary from a set of strings in lexicographical order.
  Trie(const std::vector<CharRange>& strings) {
    TrieBuilder builder{strings, BcType::kFirstBits};

    BcType{builder.bc_}.swap(bc_);
    BitVector{builder.terms_, true}.swap(terms_);
    tail_ = std::move(builder.tail_);
    alphabet_ = std::move(builder.alphabet_);
    table_ = builder.table_;

    num_strings_ = strings.size();
    max_length_ = builder.max_length_;
  }

  ~Trie() {}

  // Returns the string ID if stored, otherwise 'kNotFound'.
  uint32_t lookup(CharRange string) const {
    auto node_id = kRootId;

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

  // Return the corresponding string, access(lookup())
  std::string access(uint32_t id) const {
    if (num_strings_ <= id) {
      return {};
    }

    std::string ret;
    ret.reserve(max_length_);

    auto node_id = to_node_id_(id);
    const auto link = bc_.is_leaf(node_id) ? bc_.link(node_id) : kNotFound;

    while (node_id != kRootId) {
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

  // Return the corresponding string.
  size_t common_prefix_lookup(CharRange string, std::vector<uint32_t>& ids,
                              size_t limit = kDefaultLimit) const {
    if (limit == 0) {
      return 0;
    }

    auto node_id = kRootId;
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

  // Return the corresponding string.
  size_t predictive_lookup(CharRange string, std::vector<uint32_t>& ids,
                           size_t limit = kDefaultLimit) const {
    auto node_id = kRootId;

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
  size_t size_in_bytes() const {
    size_t ret = 0;
    ret += bc_.size_in_bytes();
    ret += terms_.size_in_bytes();
    ret += util::size_in_bytes(tail_);
    ret += util::size_in_bytes(alphabet_);
    ret += sizeof(table_);
    ret += sizeof(num_strings_);
    ret += sizeof(max_length_);
    return ret;
  }

  // Dumps statistics of the dictionary.
  void show_stat(std::ostream& os) const {
    const auto total_size = size_in_bytes();
    os << "basic statistics of xcdat::Trie" << std::endl;
    util::show_stat("\tnum strings:   ", num_strings(), os);
    util::show_stat("\talphabet size: ", alphabet_size(), os);
    util::show_stat("\tnum nodes:     ", num_nodes(), os);
    util::show_stat("\tnum used_nodes:", num_used_nodes(), os);
    util::show_stat("\tnum free_nodes:", num_free_nodes(), os);
    util::show_stat("\tsize in bytes: ", size_in_bytes(), os);
    os << "member size statistics of xcdat::Trie" << std::endl;
    util::show_stat("\tbc_:      ", bc_.size_in_bytes(), total_size, os);
    util::show_stat("\tterms_:   ", terms_.size_in_bytes(), total_size, os);
    util::show_stat("\ttail_:    ", util::size_in_bytes(tail_), total_size, os);
    util::show_stat("\talphabet_:", util::size_in_bytes(alphabet_), total_size, os);
    util::show_stat("\ttable_:   ", sizeof(table_), total_size, os);
    bc_.show_stat(os);
  }

  void write(std::ostream& os) const {
    bc_.write(os);
    terms_.write(os);
    util::write_vector(tail_, os);
    util::write_vector(alphabet_, os);
    util::write_value(table_, os);
    util::write_value(num_strings_, os);
    util::write_value(max_length_, os);
  }

  void read(std::istream& is) {
    bc_.read(is);
    terms_.read(is);
    util::read_vector(tail_, is);
    util::read_vector(alphabet_, is);
    util::read_value(table_, is);
    util::read_value(num_strings_, is);
    util::read_value(max_length_, is);
  }

  void swap(Type& rhs) {
    bc_.swap(rhs.bc_);
    terms_.swap(rhs.terms_);
    tail_.swap(rhs.tail_);
    alphabet_.swap(rhs.alphabet_);
    table_.swap(rhs.table_);
    std::swap(num_strings_, rhs.num_strings_);
    std::swap(max_length_, rhs.max_length_);
  }

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

  bool match_(CharRange string, uint32_t link) const {
    auto tail = tail_.data() + link;
    for (auto it = string.begin; it != string.end; ++it, ++tail) {
      if (*tail == '\0' || *it != *tail) {
        return false;
      }
    }
    return *tail == '\0';
  }

  bool prefix_match_(CharRange string, uint32_t link) const {
    auto tail = tail_.data() + link;
    for (auto it = string.begin; it != string.end; ++it, ++tail) {
      if (*tail == '\0' || *it != *tail) {
        return false;
      }
    }
    return true;
  }

  void enumerate_ids_(uint32_t node_id, std::vector<uint32_t>& ids,
                      size_t& num_ids, size_t limit) const {
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
};

} //namespace - xcdat

#endif //XCDAT_TRIE_HPP_
