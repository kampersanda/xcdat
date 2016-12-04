#ifndef XCDAT_TRIE_BUILDER_HPP_
#define XCDAT_TRIE_BUILDER_HPP_

#include "BitVectorBuilder.hpp"
#include "CharRange.hpp"
#include "Exception.hpp"

namespace xcdat {

template<bool Fast>
class Trie; // prototype declaration for friend

//
class TrieBuilder {
public:
  friend class Trie<true>;
  friend class Trie<false>;

  static constexpr uint32_t kTabooId = 1; // for avoiding undefined traversal
  static constexpr uint32_t kBcUpper = (1U << 31) - 1;
  static constexpr uint32_t kFreeBlocks = 16; // inspired by darts-clone

  TrieBuilder(const std::vector<CharRange>& strings, uint32_t first_bit_size);
  ~TrieBuilder() {}

  TrieBuilder(const TrieBuilder&) = delete;
  TrieBuilder& operator=(const TrieBuilder&) = delete;

private:
  struct Suffix {
    CharRange string;
    uint32_t node_id;
    std::reverse_iterator<CharRange::Type> rbegin() const {
      return std::reverse_iterator<CharRange::Type>(string.end);
    }
    std::reverse_iterator<CharRange::Type> rend() const {
      return std::reverse_iterator<CharRange::Type>(string.begin);
    }
  };

  const std::vector<CharRange>& strings_;
  const uint32_t block_size_;

  std::vector<BcElement> bc_;
  BitVectorBuilder terms_;
  std::vector<uint8_t> tail_;
  std::vector<uint8_t> alphabet_;
  std::array<uint8_t, 512> table_;

  std::vector<uint8_t> edges_;
  std::vector<uint32_t> heads_;
  std::vector<Suffix> suffixes_;

  size_t max_length_ = 0;

  void build_table_();
  void build_bc_(size_t begin, size_t end, size_t depth, uint32_t node_id);
  void build_tail_();

  void expand_();
  void use_(uint32_t node_id);
  void close_block_(uint32_t block_id);
  uint32_t find_base_(uint32_t block_id) const;
  bool is_target_(uint32_t base) const;
  void append_tail_(size_t begin, size_t end, const CharRange& string);
};

} //namespace - xcdat

#endif //XCDAT_TRIE_BUILDER_HPP_
