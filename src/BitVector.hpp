#ifndef XCDAT_BIT_VECTOR_HPP_
#define XCDAT_BIT_VECTOR_HPP_

#include "BitVectorBuilder.hpp"
#include "Vector.hpp"

namespace xcdat {

/*
 * Bit vector supporting Rank/Select operations.
 * */
class BitVector {
public:
  BitVector() {}
  // builder.width_ is stolen.
  BitVector(BitVectorBuilder& builder, bool rank_flag, bool select_flag);

  ~BitVector() {}

  bool operator[](size_t i) const {
    return (bits_[i / 32] & (1U << (i % 32))) != 0;
  }

  id_type rank(size_t i) const; // the number of 1s in B[0,i).
  id_type select(size_t i) const; // the position of the i+1 th occurrence.

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
  static constexpr id_type kBitsInR1 = 256;
  static constexpr id_type kBitsInR2 = 32;
  static constexpr id_type kR1PerR2 = kBitsInR1 / kBitsInR2; // 8
  static constexpr id_type kNum1sPerTip = 512;

  struct RankTip {
    id_type L1;
    std::array<uint8_t, kR1PerR2> L2;
  };

  Vector<uint32_t> bits_;
  Vector<RankTip> rank_tips_;
  Vector<id_type> select_tips_;
  size_t size_ = 0;
  size_t num_1s_ = 0;
};

} //namespace - xcdat

#endif //XCDAT_BIT_VECTOR_HPP_
