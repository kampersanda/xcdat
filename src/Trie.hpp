#ifndef XCDAT_TRIE_HPP_
#define XCDAT_TRIE_HPP_

#include "DacBc.hpp"
#include "FastDacBc.hpp"
#include "TrieBuilder.hpp"

namespace xcdat {

constexpr auto kNotFound = static_cast<id_type>(-1);
constexpr auto kDefaultLimit = static_cast<size_t>(-1);

/*
 * Compressed string dictionary using an improved double-array trie. There are two versions for
 * representing BASE/CHECK arrays.
 *  @param Fast: the version of DACs representing BASE/CHECK arrays.
 * */
template<bool Fast>
class Trie {
public:
  using Type = Trie<Fast>;
  using BcType = Conditional<Fast, FastDacBc, DacBc>;

  /*
   * Generic constructor.
   * */
  Trie() {}

  /*
   * Builds the dictionary from given string keys. The keys must be sorted in lexicographical order
   * without duplication. Any error in construction is reported by TrieBuilder::Exception.
   *  @param keys: the pairs of key pointers and lengths
   *  @param binary_mode: whether the keys include the ASCII zero code or not
   * */
  Trie(const std::vector<std::pair<const uint8_t*, size_t>>& keys, bool binary_mode = false) {
    TrieBuilder builder(keys, BcType::kWidthL1, false);

    BcType(builder.bc_, builder.leaf_flags_).swap(bc_);
    BitVector(builder.term_flags_, true, true).swap(terminal_flags_);
    tail_.steal(builder.tail_);
    BitVector(builder.boundary_flags_, false, false).swap(boundary_flags_);
    alphabet_.steal(builder.alphabet_);
    table_ = builder.table_;

    num_keys_ = keys.size();
    max_length_ = builder.max_length_;
    binary_mode_ = builder.binary_mode_;
  }

  /*
   * Generic destructor.
   * */
  ~Trie() {}

  /*
   * Lookups the ID of a given key.
   *  @param key: key pointer.
   *  @param length: key length.
   *  @returns the ID if the query is registered; otherwise returns kNotFound.
   * */
  id_type lookup(const uint8_t* key, size_t length) const {
    id_type node_id = 0;
    size_t i = 0;

    while (!bc_.is_leaf(node_id)) {
      if (i == length) {
        return terminal_flags_[node_id] ? to_string_id_(node_id) : kNotFound;
      }
      const auto child_id = bc_.base(node_id) ^table_[key[i++]];
      if (bc_.check(child_id) != node_id) {
        return kNotFound;
      }
      node_id = child_id;
    }

    if (match_(key + i, length - i, bc_.link(node_id))) {
      return to_string_id_(node_id);
    }
    return kNotFound;
  }

  /*
   * Decodes the key associated with a given ID.
   *  @param id: ID.
   *  @param[out] ret: the decoded key.
   *  @returns whether the given ID is within the range or not.
   * */
  bool access(id_type id, std::vector<uint8_t>& ret) const {
    if (num_keys_ <= id) {
      return false;
    }

    ret.reserve(ret.size() + max_length_);

    auto node_id = to_node_id_(id);
    auto link = bc_.is_leaf(node_id) ? bc_.link(node_id) : kNotFound;

    while (node_id) {
      const auto parent_id = bc_.check(node_id);
      ret.emplace_back(edge_(parent_id, node_id));
      node_id = parent_id;
    }

    std::reverse(std::begin(ret), std::end(ret));

    if (link != 0 && link != kNotFound) {
      if (binary_mode_) {
        do {
          ret.push_back(tail_[link]);
        } while (!boundary_flags_[link++]);
      } else {
        do {
          ret.push_back(tail_[link++]);
        } while (tail_[link]);
      }
    }

    return true;
  }

  /*
   * Enumerates the IDs of keys included as prefixes of a given key.
   *  @param key: key pointer.
   *  @param length: key length.
   *  @param[out] ids: IDs of matched keys.
   *  @param limit: the maximum number of matched keys (optional).
   *  @returns the number of matched keys.
   * */
  size_t common_prefix_lookup(const uint8_t* key, size_t length, std::vector<id_type>& ids,
                              size_t limit = kDefaultLimit) const {
    if (limit == 0) {
      return 0;
    }

    id_type node_id = 0;
    size_t i = 0, num_ids = 0;

    while (!bc_.is_leaf(node_id)) {
      if (terminal_flags_[node_id]) {
        ids.push_back(to_string_id_(node_id));
        ++num_ids;
        if (num_ids == limit) {
          return num_ids;
        }
      }
      if (i == length) {
        return num_ids;
      }

      const auto child_id = bc_.base(node_id) ^table_[key[i++]];

      if (bc_.check(child_id) != node_id) {
        return num_ids;
      }

      node_id = child_id;
    }

    if (match_(key + i, length - i, bc_.link(node_id))) {
      ids.push_back(to_string_id_(node_id));
      ++num_ids;
    }

    return num_ids;
  }

  /*
   * Enumerates the IDs of keys starting with a given key.
   *  @param key: key pointer.
   *  @param length: key length.
   *  @param[out] ids: IDs of matched keys.
   *  @param limit: the maximum number of matched keys (optional).
   *  @returns the number of matched keys.
   * */
  size_t predictive_lookup(const uint8_t* key, size_t length, std::vector<id_type>& ids,
                           size_t limit = kDefaultLimit) const {
    if (limit == 0) {
      return 0;
    }

    id_type node_id = 0;
    size_t i = 0;

    for (; i < length; ++i) {
      if (bc_.is_leaf(node_id)) {
        if (prefix_match_(key + i, length - i, bc_.link(node_id))) {
          ids.push_back(to_string_id_(node_id));
          return 1;
        }
        return 0;
      }

      const auto child_id = bc_.base(node_id) ^table_[key[i]];
      if (bc_.check(child_id) != node_id) {
        return 0;
      }
      node_id = child_id;
    }

    size_t num_ids = 0;
    enumerate_ids_(node_id, ids, num_ids, limit);
    return num_ids;
  }

  /*
   * Gets the number of keys in the dictionary.
   *  @returns the number of keys in the dictionary.
   * */
  size_t num_keys() const {
    return num_keys_;
  }

  /*
   * Gets the size of alphabet drawing keys in the dictionary.
   *  @returns the alphabet size.
   * */
  size_t alphabet_size() const {
    return alphabet_.size();
  }

  /*
   * Gets the number of nodes assigned by arranging nodes.
   * The result is the same as num_used_nodes() + num_free_nodes().
   *  @returns the number of the nodes.
   * */
  size_t num_nodes() const {
    return bc_.num_nodes();
  }

  /*
   * Gets the number of nodes in the original trie.
   *  @returns the number of the nodes.
   * */
  size_t num_used_nodes() const {
    return bc_.num_used_nodes();
  }

  /*
   * Gets the number of nodes corresponding to empty elements.
   *  @returns the number of the nodes.
   * */
  size_t num_free_nodes() const {
    return bc_.num_free_nodes();
  }

  /*
   * Computes the size of the structure in bytes.
   *  @returns the dictionary size in bytes.
   * */
  size_t size_in_bytes() const {
    size_t ret = 0;
    ret += bc_.size_in_bytes();
    ret += terminal_flags_.size_in_bytes();
    ret += tail_.size_in_bytes();
    ret += boundary_flags_.size_in_bytes();
    ret += alphabet_.size_in_bytes();
    ret += sizeof(table_);
    ret += sizeof(num_keys_);
    ret += sizeof(max_length_);
    return ret;
  }

  /*
   * Gets the binary mode.
   *  @returns the binary mode.
   * */
  bool is_binary_mode() const {
    return binary_mode_;
  }

  /*
   * Reports the dictionary statistics into an ostream.
   *  @param os: the ostream.
   * */
  void show_stat(std::ostream& os) const {
    const auto total_size = size_in_bytes();
    os << "basic statistics of xcdat::Trie" << std::endl;
    show_size("\tnum keys:      ", num_keys(), os);
    show_size("\talphabet size: ", alphabet_size(), os);
    show_size("\tnum nodes:     ", num_nodes(), os);
    show_size("\tnum used nodes:", num_used_nodes(), os);
    show_size("\tnum free nodes:", num_free_nodes(), os);
    show_size("\tsize in bytes: ", size_in_bytes(), os);
    os << "member size statistics of xcdat::Trie" << std::endl;
    show_size_ratio("\tbc:            ", bc_.size_in_bytes(), total_size, os);
    show_size_ratio("\tterminal_flags:", terminal_flags_.size_in_bytes(), total_size, os);
    show_size_ratio("\ttail:          ", tail_.size_in_bytes(), total_size, os);
    show_size_ratio("\tboundary_flags:", boundary_flags_.size_in_bytes(), total_size, os);
    bc_.show_stat(os);
  }

  /*
   * Writes the dictionary into an ostream.
   *  @param os: the ostream.
   * */
  void write(std::ostream& os) const {
    bc_.write(os);
    terminal_flags_.write(os);
    tail_.write(os);
    boundary_flags_.write(os);
    alphabet_.write(os);
    write_value(table_, os);
    write_value(num_keys_, os);
    write_value(max_length_, os);
  }

  /*
   * Reads the dictionary from an istream.
   *  @param is: the istream.
   * */
  void read(std::istream& is) {
    bc_.read(is);
    terminal_flags_.read(is);
    tail_.read(is);
    boundary_flags_.read(is);
    alphabet_.read(is);
    read_value(table_, is);
    read_value(num_keys_, is);
    read_value(max_length_, is);
  }

  /*
   * Swaps the dictionary.
   *  @param rhs: the dictionary to be swapped.
   * */
  void swap(Type& rhs) {
    bc_.swap(rhs.bc_);
    terminal_flags_.swap(rhs.terminal_flags_);
    tail_.swap(rhs.tail_);
    boundary_flags_.swap(rhs.boundary_flags_);
    alphabet_.swap(rhs.alphabet_);
    table_.swap(rhs.table_);
    std::swap(num_keys_, rhs.num_keys_);
    std::swap(max_length_, rhs.max_length_);
  }

  /*
   * Disallows copy and assignment.
   * */
  Trie(const Trie&) = delete;
  Trie& operator=(const Trie&) = delete;

private:
  BcType bc_;
  BitVector terminal_flags_;
  Vector<uint8_t> tail_;
  BitVector boundary_flags_; // if binary_mode_
  Vector<uint8_t> alphabet_;
  std::array<uint8_t, 512> table_; // table[table[c] + 256] = c

  size_t num_keys_ = 0;
  size_t max_length_ = 0;
  bool binary_mode_ = false;

  id_type to_string_id_(id_type node_id) const {
    return terminal_flags_.rank(node_id);
  };

  id_type to_node_id_(id_type string_id) const {
    return terminal_flags_.select(string_id);
  };

  uint8_t edge_(id_type node_id, id_type child_id) const {
    return table_[static_cast<uint8_t>(bc_.base(node_id) ^ child_id) + 256];
  }

  bool match_(const uint8_t* key, size_t length, id_type link) const {
    if (link == 0) {
      return length == 0;
    }

    if (binary_mode_) {
      for (size_t i = 0; i < length;) {
        if (tail_[link] != key[i++]) {
          return false;
        }
        if (boundary_flags_[link++]) {
          return i == length;
        }
      }
      return false;
    } else {
      auto tail = tail_.data() + link;
      for (size_t i = 0; i < length; ++i) {
        if (tail[i] == '\0' || key[i] != tail[i]) {
          return false;
        }
      }
      return tail[length] == '\0';
    }
  }

  bool prefix_match_(const uint8_t* key, size_t length, id_type link) const {
    if (link == 0) {
      return length == 0;
    }

    if (binary_mode_) {
      for (size_t i = 0; i < length;) {
        if (tail_[link] != key[i++]) {
          return false;
        }
        if (boundary_flags_[link++]) {
          return i == length;
        }
      }
    } else {
      auto tail = tail_.data() + link;
      for (size_t i = 0; i < length; ++i) {
        if (tail[i] == '\0' || key[i] != tail[i]) {
          return false;
        }
      }
    }

    return true;
  }

  void enumerate_ids_(id_type node_id, std::vector<id_type>& ids,
                      size_t& num_ids, size_t limit) const {
    if (terminal_flags_[node_id]) {
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
      const auto child_id = base ^table_[label];
      if (bc_.check(child_id) == node_id) {
        enumerate_ids_(child_id, ids, num_ids, limit);
      }
    }
  }
};

} //namespace - xcdat

#endif //XCDAT_TRIE_HPP_
