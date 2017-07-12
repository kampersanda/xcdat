#ifndef XCDAT_TRIE_HPP_
#define XCDAT_TRIE_HPP_

#include "DacBc.hpp"
#include "FastDacBc.hpp"

namespace xcdat {

constexpr auto kNotFound = kIdMax;

// Compressed string dictionary using an improved double-array trie. There are two versions of DACs
// representing BASE/CHECK arrays, selected with the Fast parameter.
template<bool Fast>
class Trie {
public:
  using Type = Trie<Fast>;
  using BcType = typename std::conditional<Fast, FastDacBc, DacBc>::type;

  // Generic constructor.
  Trie() {}

  // Reads the dictionary from an istream.
  Trie(std::istream& is) {
    bc_ = BcType(is);
    terminal_flags_ = BitVector(is);
    tail_ = Vector<uint8_t>(is);
    boundary_flags_ = BitVector(is);
    alphabet_ = Vector<uint8_t>(is);
    is.read(reinterpret_cast<char*>(table_), 512);
    num_keys_ = read_value<size_t>(is);
    max_length_ = read_value<size_t>(is);
    binary_mode_ = read_value<bool>(is);
  }

  // Generic destructor.
  ~Trie() {}

  // Lookups the ID of a given key. If the key is not registered, returns kNotFound.
  id_type lookup(const uint8_t* key, size_t length) const {
    size_t pos = 0;
    id_type node_id = 0;

    while (!bc_.is_leaf(node_id)) {
      if (pos == length) {
        return terminal_flags_[node_id] ? to_key_id_(node_id) : kNotFound;
      }

      const auto child_id = bc_.base(node_id) ^table_[key[pos++]];
      if (bc_.check(child_id) != node_id) {
        return kNotFound;
      }

      node_id = child_id;
    }

    size_t tail_pos = bc_.link(node_id);
    if (!match_(key, length, pos, tail_pos)) {
      return kNotFound;
    }

    return to_key_id_(node_id);
  }

  // Decodes the key associated with a given ID. The decoded key is appended to 'ret' and its
  // length is returned.
  size_t access(id_type id, std::vector<uint8_t>& ret) const {
    if (num_keys_ <= id) {
      return 0;
    }

    auto orig_size = ret.size();
    ret.reserve(orig_size + max_length_);

    auto node_id = to_node_id_(id);
    auto tail_pos = bc_.is_leaf(node_id) ? bc_.link(node_id) : kNotFound;

    while (node_id) {
      const auto parent_id = bc_.check(node_id);
      ret.push_back(edge_(parent_id, node_id));
      node_id = parent_id;
    }

    std::reverse(std::begin(ret) + orig_size, std::end(ret));

    if (tail_pos != 0 && tail_pos != kNotFound) {
      if (binary_mode_) {
        do {
          ret.push_back(tail_[tail_pos]);
        } while (!boundary_flags_[tail_pos++]);
      } else {
        do {
          ret.push_back(tail_[tail_pos++]);
        } while (tail_[tail_pos]);
      }
    }

    return ret.size() - orig_size;
  }

  // Returns the IDs of keys included as prefixes of a given key. The IDs are appended to 'ids' and
  // the number is returned. By using 'limit', you can restrict the maximum number of returned IDs.
  size_t common_prefix_lookup(const uint8_t* key, size_t length, std::vector<id_type>& ids,
                              size_t limit = std::numeric_limits<size_t>::max()) const {
    if (limit == 0) {
      return 0;
    }

    size_t pos = 0, count = 0;
    id_type node_id = 0;

    while (!bc_.is_leaf(node_id)) {
      if (terminal_flags_[node_id]) {
        ids.push_back(to_key_id_(node_id));
        if (limit <= ++count) {
          return count;
        }
      }

      if (pos == length) {
        return count;
      }

      const auto child_id = bc_.base(node_id) ^table_[key[pos++]];
      if (bc_.check(child_id) != node_id) {
        return count;
      }

      node_id = child_id;
    }

    size_t tail_pos = bc_.link(node_id);
    if (match_(key, length, pos, tail_pos)) {
      ids.push_back(to_key_id_(node_id));
      ++count;
    }

    return count;
  }

  // Returns the IDs of keys starting with a given key. The IDs are appended to 'ids' and the
  // number is returned. By using 'limit', you can restrict the maximum number of returned IDs.
  size_t predictive_lookup(const uint8_t* key, size_t length, std::vector<id_type>& ids,
                           size_t limit = std::numeric_limits<size_t>::max()) const {
    if (limit == 0) {
      return 0;
    }

    size_t pos = 0;
    id_type node_id = 0;

    for (; pos < length; ++pos) {
      if (bc_.is_leaf(node_id)) {
        size_t tail_pos = bc_.link(node_id);
        if (!prefix_match_(key, length, pos, tail_pos)) {
          return 0;
        }

        ids.push_back(to_key_id_(node_id));
        return 1;
      }

      const auto child_id = bc_.base(node_id) ^table_[key[pos]];
      if (bc_.check(child_id) != node_id) {
        return 0;
      }

      node_id = child_id;
    }

    size_t count = 0;

    std::vector<std::pair<id_type, size_t>> stack;
    stack.push_back({node_id, pos});

    while (!stack.empty()) {
      node_id = stack.back().first;
      pos = stack.back().second;
      stack.pop_back();

      if (bc_.is_leaf(node_id)) {
        ids.push_back(to_key_id_(node_id));
        if (limit <= ++count) {
          break;
        }
      } else {
        if (terminal_flags_[node_id]) {
          ids.push_back(to_key_id_(node_id));
          if (limit <= ++count) {
            break;
          }
        }

        const auto base = bc_.base(node_id);
        for (const auto label : alphabet_) {
          const auto child_id = base ^table_[label];
          if (bc_.check(child_id) == node_id) {
            stack.push_back({child_id, pos + 1});
          }
        }
      }
    }

    return count;
  }

  // Gets the number of registered keys in the dictionary
  size_t num_keys() const {
    return num_keys_;
  }

  // Gets the maximum length of registered keys
  size_t max_length() const {
    return max_length_;
  }

  // Gets the binary mode
  bool is_binary_mode() const {
    return binary_mode_;
  }

  // Gets the size of alphabet drawing keys in the dictionary.
  size_t alphabet_size() const {
    return alphabet_.size();
  }

  // Gets the number of nodes including free nodes.
  size_t num_nodes() const {
    return bc_.num_nodes();
  }

  // Gets the number of nodes in the original trie.
  size_t num_used_nodes() const {
    return bc_.num_used_nodes();
  }

  // Gets the number of free nodes corresponding to empty elements.
  size_t num_free_nodes() const {
    return bc_.num_free_nodes();
  }

  // Computes the output dictionary size in bytes.
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
    ret += sizeof(binary_mode_);
    return ret;
  }

  // Reports the dictionary statistics into an ostream.
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

  // Writes the dictionary into an ostream.
  void write(std::ostream& os) const {
    bc_.write(os);
    terminal_flags_.write(os);
    tail_.write(os);
    boundary_flags_.write(os);
    alphabet_.write(os);
    os.write(reinterpret_cast<const char*>(table_), 512);
    write_value(num_keys_, os);
    write_value(max_length_, os);
    write_value(binary_mode_, os);
  }

  // Disallows copy and assignment.
  Trie(const Trie&) = delete;
  Trie& operator=(const Trie&) = delete;

  Trie(Trie&&) = default;
  Trie& operator=(Trie&&) = default;

private:
  BcType bc_;
  BitVector terminal_flags_;
  Vector<uint8_t> tail_;
  BitVector boundary_flags_; // used if binary_mode_ == true
  Vector<uint8_t> alphabet_;
  uint8_t table_[512]; // table[table[c] + 256] = c

  size_t num_keys_ = 0;
  size_t max_length_ = 0;
  bool binary_mode_ = false;

  id_type to_key_id_(id_type node_id) const {
    return terminal_flags_.rank(node_id);
  };

  id_type to_node_id_(id_type string_id) const {
    return terminal_flags_.select(string_id);
  };

  uint8_t edge_(id_type node_id, id_type child_id) const {
    return table_[static_cast<uint8_t>(bc_.base(node_id) ^ child_id) + 256];
  }

  bool match_(const uint8_t* key, size_t length, size_t pos, size_t tail_pos) const {
    assert(pos <= length);

    if (pos == length) {
      return tail_pos == 0;
    }

    if (binary_mode_) {
      do {
        if (key[pos] != tail_[tail_pos]) {
          return false;
        }
        ++pos;
        if (boundary_flags_[tail_pos]) {
          return pos == length;
        }
        ++tail_pos;
      } while (pos < length);
      return false;
    } else {
      do {
        if (!tail_[tail_pos] || key[pos] != tail_[tail_pos]) {
          return false;
        }
        ++pos;
        ++tail_pos;
      } while (pos < length);
      return !tail_[tail_pos];
    }
  }

  bool prefix_match_(const uint8_t* key, size_t length, size_t pos, size_t tail_pos) const {
    assert(pos < length);

    if (tail_pos == 0) {
      return false;
    }

    if (binary_mode_) {
      do {
        if (key[pos] != tail_[tail_pos]) {
          return false;
        }
        ++pos;
        if (boundary_flags_[tail_pos]) {
          return pos == length;
        }
        ++tail_pos;
      } while (pos < length);
    } else {
      do {
        if (key[pos] != tail_[tail_pos] || !tail_[tail_pos]) {
          return false;
        }
        ++pos;
        ++tail_pos;
      } while (pos < length);
    }
    return true;
  }

  friend class TrieBuilder;
};

} //namespace - xcdat

#endif //XCDAT_TRIE_HPP_
