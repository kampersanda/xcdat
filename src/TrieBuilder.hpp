#ifndef XCDAT_TRIE_BUILDER_HPP_
#define XCDAT_TRIE_BUILDER_HPP_

#include "Trie.hpp"

namespace xcdat {

// Double-array trie builder.
class TrieBuilder {
public:
  // for avoiding undefined traversal
  static constexpr id_type kTabooId = 1;
  // inspired by darts-clone
  static constexpr id_type kFreeBlocks = 16;

  struct Key {
    const uint8_t* ptr;
    size_t length;
  };

  // Builds the dictionary from given string keys. The keys must be sorted in lexicographical order
  // without duplication. Any error in construction is reported by TrieBuilder::Exception. If the
  // keys include the ASCII zero code, pass binary_mode = true.
  template<bool Fast>
  static Trie<Fast> build(const std::vector<Key>& keys, bool binary_mode = false) {
    TrieBuilder builder(keys, Trie<Fast>::BcType::kWidthL1, binary_mode);

    Trie<Fast> trie;

    trie.bc_ = typename Trie<Fast>::BcType(builder.bc_, builder.leaf_flags_);
    trie.terminal_flags_ = BitVector(builder.term_flags_, true, true);
    trie.tail_ = Vector<uint8_t>(builder.tail_);
    trie.boundary_flags_ = BitVector(builder.boundary_flags_, false, false);
    trie.alphabet_ = builder.alphabet_;
    std::swap(trie.table_, builder.table_);

    trie.num_keys_ = keys.size();
    trie.max_length_ = builder.max_length_;
    trie.binary_mode_ = builder.binary_mode_;

    return trie;
  }

  // Exception class for xcdat::TrieBuilder
  class Exception : public std::exception {
  public:
    explicit Exception(std::string message) : message_(message) {}
    virtual ~Exception() throw() {}

    // overrides what() of std::exception.
    virtual const char* what() const throw() {
      return message_.c_str();
    }

  private:
    std::string message_;
  };

  TrieBuilder(const TrieBuilder&) = delete;
  TrieBuilder& operator=(const TrieBuilder&) = delete;

private:
  struct Suffix {
    Key str;
    id_type node_id;

    size_t length() const {
      return str.length;
    }
    uint8_t operator[](size_t i) const {
      return str.ptr[length() - i - 1];
    }

    std::reverse_iterator<const uint8_t*> rbegin() const {
      return std::reverse_iterator<const uint8_t*>(str.ptr + str.length);
    }
    std::reverse_iterator<const uint8_t*> rend() const {
      return std::reverse_iterator<const uint8_t*>(str.ptr);
    }
  };

  const std::vector<Key>& keys_;
  const id_type block_size_;
  const id_type width_L1_;

  bool binary_mode_ = false;

  std::vector<BcPair> bc_;
  BitVectorBuilder leaf_flags_;
  BitVectorBuilder term_flags_;
  std::vector<uint8_t> tail_;
  BitVectorBuilder boundary_flags_;
  std::vector<uint8_t> alphabet_;
  uint8_t table_[512];

  std::vector<bool> used_flags_;
  std::vector<uint8_t> edges_;
  std::vector<id_type> heads_;
  std::vector<Suffix> suffixes_;

  size_t max_length_ = 0;

  TrieBuilder(const std::vector<Key>& keys, id_type width_L1, bool binary_mode);
  ~TrieBuilder() {}

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
