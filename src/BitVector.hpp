#ifndef XCDAT_BIT_VECTOR_HPP_
#define XCDAT_BIT_VECTOR_HPP_

#include "BitVectorBuilder.hpp"

namespace xcdat {

/*
 * Bit vector supporting Rank/Select operations.
 * */
class BitVector {
public:
  BitVector() {}
  BitVector(BitVectorBuilder& builder, bool select_flag = false); // builder.bits_ is stolen.

  ~BitVector() {}

  bool operator[](uint32_t i) const {
    return (bits_[i / 32] & (1U << (i % 32))) != 0;
  }

  uint32_t rank(uint32_t i) const; // the number of 1s in B[0,i).
  uint32_t select(uint32_t i) const; // the position of the i+1 th occurrence.

  size_t num_1s() const {
    return num_1s_;
  }
  size_t num_0s() const {
    return size_ - num_1s_;
  }

  size_t size() const { // the number of bits
    return size_;
  }
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
