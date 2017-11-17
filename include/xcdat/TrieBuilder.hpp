#ifndef XCDAT_TRIE_BUILDER_HPP_
#define XCDAT_TRIE_BUILDER_HPP_

#include "Trie.hpp"

namespace xcdat {

// Double-array trie builder.
class TrieBuilder {
public:
  // Builds the dictionary from given string keys. The keys must be sorted in
  // lexicographical order without duplication. Any error in construction is
  // reported by TrieBuilder::Exception. If the keys include the ASCII zero
  // code, pass bin_mode = true.
  template<bool Fast>
  static Trie<Fast>
  build(const std::vector<std::string_view>& keys, bool bin_mode = false) {
    TrieBuilder builder(keys, Trie<Fast>::bc_type::WIDTH_L1, bin_mode);

    Trie<Fast> trie;

    trie.bc_ = typename Trie<Fast>::bc_type(builder.bc_, builder.leaf_flags_);
    trie.terminal_flags_ = BitVector(builder.term_flags_, true, true);
    trie.tail_ = Vector<char>(builder.tail_);
    trie.boundary_flags_ = BitVector(builder.boundary_flags_, false, false);
    trie.alphabet_ = Vector<uint8_t>(builder.alphabet_);
    std::swap(trie.table_, builder.table_);

    trie.num_keys_ = keys.size();
    trie.max_length_ = builder.max_length_;
    trie.bin_mode_ = builder.bin_mode_;

    return trie;
  }

  // Exception class for xcdat::TrieBuilder
  class Exception : public std::exception {
  public:
    explicit Exception(std::string message) : message_(std::move(message)) {}
    ~Exception() throw() override {};

    // overrides what() of std::exception.
    const char* what() const throw() override {
      return message_.c_str();
    }

  private:
    std::string message_;
  };

  TrieBuilder(const TrieBuilder&) = delete;
  TrieBuilder& operator=(const TrieBuilder&) = delete;

private:
  struct Suffix {
    std::string_view str;
    id_type node_id;

    size_t length() const {
      return str.length();
    }
    char operator[](size_t i) const {
      return str[length() - i - 1];
    }

    std::reverse_iterator<const char*> rbegin() const {
      return std::make_reverse_iterator(str.data() + str.length());
    }
    std::reverse_iterator<const char*> rend() const {
      return std::make_reverse_iterator(str.data());
    }
  };

  // To avoid undefined traversal
  static constexpr id_type TABOO_ID = 1;
  // From darts-clone setting
  static constexpr id_type FREE_BLOCKS = 16;

  const std::vector<std::string_view>& keys_;
  const id_type block_size_;
  const id_type width_L1_;

  bool bin_mode_{};

  std::vector<BcPair> bc_{};
  BitVectorBuilder leaf_flags_{};
  BitVectorBuilder term_flags_{};
  std::vector<char> tail_{};
  BitVectorBuilder boundary_flags_{};
  std::vector<uint8_t> alphabet_{};
  uint8_t table_[512]{};

  std::vector<bool> used_flags_{};
  std::vector<uint8_t> edges_{};
  std::vector<id_type> heads_{};
  std::vector<Suffix> suffixes_{};

  size_t max_length_{};

  TrieBuilder(const std::vector<std::string_view>& keys,
              id_type width_L1, bool bin_mode);
  ~TrieBuilder() = default;

  void build_table_();
  void build_bc_(size_t begin, size_t end, size_t depth, id_type node_id);
  void build_tail_();

  void expand_();
  void use_(id_type node_id);
  void close_block_(id_type block_id);
  id_type find_base_(id_type block_id) const;
  bool is_target_(id_type base) const;
};

} //namespace - xcdat

#endif //XCDAT_TRIE_BUILDER_HPP_
