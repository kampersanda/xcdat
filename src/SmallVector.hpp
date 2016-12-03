#ifndef XCDAT_SMALL_VECTOR_HPP_
#define XCDAT_SMALL_VECTOR_HPP_

#include "xcdatBasics.hpp"

namespace xcdat {

class SmallVector {
public:
  SmallVector() {}
  SmallVector(const std::vector<uint32_t>& integers);

  ~SmallVector() {}

  uint32_t operator[](uint32_t i) const {
    auto chunk_pos = i * bits_ / 32;
    auto offset = i * bits_ % 32;
    if (offset + bits_ <= 32) {
      return (chunks_[chunk_pos] >> offset) & mask_;
    } else {
      return ((chunks_[chunk_pos] >> offset)
              | (chunks_[chunk_pos + 1] << (32 - offset))) & mask_;
    }
  }

  size_t size() const { return size_; }
  size_t size_in_bytes() const;

  void write(std::ostream &os) const;
  void read(std::istream &is);

  void swap(SmallVector& rhs);

  SmallVector(const SmallVector&) = delete;
  SmallVector& operator=(const SmallVector&) = delete;

private:
  std::vector<uint32_t> chunks_;
  size_t size_ = 0;
  uint32_t bits_ = 0;
  uint32_t mask_ = 0;
};

} //namespace - xcdat

#endif //XCDAT_SMALL_VECTOR_HPP_
