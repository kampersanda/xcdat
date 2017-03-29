#include "FitVector.hpp"

namespace xcdat {

FitVector::FitVector(const std::vector<id_type>& integers) {
  if (integers.empty()) {
    return;
  }

  width_ = 0;
  auto max_value = *std::max_element(std::begin(integers), std::end(integers));
  do {
    ++width_;
    max_value >>= 1;
  } while (max_value);

  size_ = integers.size();
  mask_ = (1U << width_) - 1;
  std::vector<id_type> chunks(size_ * width_ / kChunkWidth + 1, 0);

  for (id_type i = 0; i < size_; ++i) {
    const auto chunk_pos = i * width_ / kChunkWidth;
    const auto offset = i * width_ % kChunkWidth;

    chunks[chunk_pos] &= ~(mask_ << offset);
    chunks[chunk_pos] |= (integers[i] & mask_) << offset;

    if (kChunkWidth < offset + width_) {
      chunks[chunk_pos + 1] &= ~(mask_ >> (kChunkWidth - offset));
      chunks[chunk_pos + 1] |= (integers[i] & mask_) >> (kChunkWidth - offset);
    }
  }

  chunks_.steal(chunks);
}

size_t FitVector::size_in_bytes() const {
  size_t ret = 0;
  ret += chunks_.size_in_bytes();
  ret += sizeof(size_);
  ret += sizeof(width_);
  ret += sizeof(mask_);
  return ret;
}

void FitVector::write(std::ostream& os) const {
  chunks_.write(os);
  write_value(size_, os);
  write_value(width_, os);
  write_value(mask_, os);
}

void FitVector::read(std::istream& is) {
  chunks_.read(is);
  read_value(size_, is);
  read_value(width_, is);
  read_value(mask_, is);
}

void FitVector::swap(FitVector& rhs) {
  chunks_.swap(rhs.chunks_);
  std::swap(size_, rhs.size_);
  std::swap(width_, rhs.width_);
  std::swap(mask_, rhs.mask_);
}

} //namespace - xcdat
