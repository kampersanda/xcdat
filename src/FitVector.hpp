#ifndef XCDAT_SMALL_VECTOR_HPP_
#define XCDAT_SMALL_VECTOR_HPP_

#include "Vector.hpp"

namespace xcdat {

// Compacted integer vector.
class FitVector {
public:
  static constexpr id_type kChunkWidth = sizeof(id_type) * 8;

  FitVector() {}
  FitVector(std::istream &is);
  FitVector(const std::vector<id_type>& values);

  ~FitVector() {}

  id_type operator[](size_t i) const {
    auto chunk_pos = static_cast<id_type>(i * width_ / kChunkWidth);
    auto offset = static_cast<id_type>(i * width_ % kChunkWidth);
    if (offset + width_ <= kChunkWidth) {
      return (chunks_[chunk_pos] >> offset) & mask_;
    } else {
      return ((chunks_[chunk_pos] >> offset)
              | (chunks_[chunk_pos + 1] << (kChunkWidth - offset))) & mask_;
    }
  }

  size_t size() const { return size_; }
  size_t size_in_bytes() const;

  void write(std::ostream &os) const;

  FitVector(const FitVector&) = delete;
  FitVector& operator=(const FitVector&) = delete;

  FitVector(FitVector&&) = default;
  FitVector& operator=(FitVector&&) = default;

private:
  Vector<id_type> chunks_;
  size_t size_ = 0;
  id_type width_ = 0;
  id_type mask_ = 0;
};

} //namespace - xcdat

#endif //XCDAT_SMALL_VECTOR_HPP_
