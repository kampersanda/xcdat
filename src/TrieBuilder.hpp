#ifndef XCDAT_TRIE_BUILDER_HPP_
#define XCDAT_TRIE_BUILDER_HPP_

#include "BitVectorBuilder.hpp"

namespace xcdat {

// prototype declaration for friend
template<bool> class Trie;

/*
 * Double-array trie builder.
 * */
class TrieBuilder {
public:
  friend class Trie<true>;
  friend class Trie<false>;

  // for avoiding undefined traversal
  static constexpr id_type kTabooId = 1;
  // inspired by darts-clone
  static constexpr id_type kFreeBlocks = 16;

  TrieBuilder(const std::vector<std::pair<const uint8_t*, size_t>>& keys,
              id_type width_L1, bool binary_mode);
  ~TrieBuilder() {}

  /*
   * Exception class for xcdat::TrieBuilder
   * */
  class Exception : public std::exception {
  public:
    explicit Exception(const std::string& message) : message_(message) {}
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
    std::pair<const uint8_t*, size_t> string;
    id_type node_id;

    size_t length() const {
      return string.second;
    }
    uint8_t operator[](size_t i) const {
      return string.first[length() - i - 1];
    }

    std::reverse_iterator<const uint8_t*> rbegin() const {
      return std::reverse_iterator<const uint8_t*>(string.first + string.second);
    }
    std::reverse_iterator<const uint8_t*> rend() const {
      return std::reverse_iterator<const uint8_t*>(string.first);
    }
  };

  const std::vector<std::pair<const uint8_t*, size_t>>& keys_;
  const id_type block_size_;
  const id_type width_L1_;

  bool binary_mode_ = false;

  std::vector<BcPair> bc_;
  BitVectorBuilder leaf_flags_;
  BitVectorBuilder term_flags_;

  std::vector<uint8_t> tail_;
  BitVectorBuilder boundary_flags_;

  std::vector<uint8_t> alphabet_;
  std::array<uint8_t, 512> table_;

  std::vector<bool> used_flags_;
  std::vector<uint8_t> edges_;
  std::vector<id_type> heads_;
  std::vector<Suffix> suffixes_;

  size_t max_length_ = 0;

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
