#ifndef XCDAT_BIT_VECTOR_HPP_
#define XCDAT_BIT_VECTOR_HPP_

#include "BitVectorBuilder.hpp"

namespace xcdat {

// A simple rank/select dictionary.
class BitVector {
public:
  BitVector() {}
  // builder.bits_ is stolen.
  BitVector(BitVectorBuilder& builder, bool select_flag = false);

  ~BitVector() {}

  bool operator[](uint32_t i) const {
    return (bits_[i / 32] & (1U << (i % 32))) != 0;
  }

  // # of 1s in B[0,i)
  uint32_t rank(uint32_t i) const;
  // position of the i+1 th occurrence
  uint32_t select(uint32_t i) const;

  size_t num_1s() const { return num_1s_; }
  size_t num_0s() const { return size_ - num_1s_; }

  size_t size() const { return size_; } // # of bits
  size_t size_in_bytes() const;

  void write(std::ostream &os) const;
  void read(std::istream &is);

  void swap(BitVector& rhs);

  BitVector(const BitVector&) = delete;
  BitVector& operator=(const BitVector&) = delete;

private:
  static constexpr uint32_t kBitsInR1 = 256;
  static constexpr uint32_t kBitsInR2 = 32;
  static constexpr uint32_t kR1PerR2 = kBitsInR1 / kBitsInR2;
  static constexpr uint32_t kNum1sPerTip = 512;

  using RankTip = std::pair<uint32_t, std::array<uint8_t, kR1PerR2>>;

  std::vector<uint32_t> bits_;
  std::vector<RankTip> rank_tips_;
  std::vector<uint32_t> select_tips_;
  size_t size_ = 0;
  size_t num_1s_ = 0;
};

} //namespace - xcdat

#endif //XCDAT_BIT_VECTOR_HPP_
