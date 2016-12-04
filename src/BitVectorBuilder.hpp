#ifndef XCDAT_BIT_VECTOR_BUILDER_HPP_
#define XCDAT_BIT_VECTOR_BUILDER_HPP_

#include "xcdatBasics.hpp"

namespace xcdat {

class BitVectorBuilder {
public:
  friend class BitVector;

  BitVectorBuilder() {}
  BitVectorBuilder(size_t size) { resize(size); }

  ~BitVectorBuilder() {}

  void push_back(bool bit) {
    if (size_ % 32 == 0) {
      bits_.emplace_back(0);
    }
    if (bit) {
      set_bit(size_, true);
    }
    ++size_;
  }

  void set_bit(size_t i, bool bit) {
    if (bit) {
      bits_[i / 32] |= (1U << (i % 32));
    } else {
      bits_[i / 32] &= (~(1U << (i % 32)));
    }
  }

  void resize(size_t size) {
    bits_.resize(size / 32 + 1, 0);
    size_ = size;
  }

  void reserve(size_t capacity) {
    bits_.reserve(capacity / 32 + 1);
  }

  size_t size() const {
    return size_;
  }

  BitVectorBuilder(const BitVectorBuilder&) = delete;
  BitVectorBuilder& operator=(const BitVectorBuilder&) = delete;

private:
  std::vector<uint32_t> bits_;
  size_t size_ = 0;
};

} //namespace - xcdat

#endif //XCDAT_BIT_VECTOR_BUILDER_HPP_
