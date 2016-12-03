#include "SmallVector.hpp"

namespace xcdat {

SmallVector::SmallVector(const std::vector<uint32_t>& integers) {
  if (integers.empty()) {
    return;
  }

  bits_ = 0;
  auto max_value = *std::max_element(std::begin(integers), std::end(integers));
  do {
    ++bits_;
    max_value >>= 1;
  } while (max_value);

  size_ = integers.size();
  chunks_.resize(size_ * bits_ / 32 + 1, 0);
  mask_ = static_cast<uint32_t>((1 << bits_) - 1);

  for (uint32_t i = 0; i < size_; ++i) {
    const auto chunk_pos = i * bits_ / 32;
    const auto offset = i * bits_ % 32;

    chunks_[chunk_pos] &= ~(mask_ << offset);
    chunks_[chunk_pos] |= (integers[i] & mask_) << offset;

    if (32 < offset + bits_) {
      chunks_[chunk_pos + 1] &= ~(mask_ >> (32 - offset));
      chunks_[chunk_pos + 1] |= (integers[i] & mask_) >> (32 - offset);
    }
  }
}

size_t SmallVector::size_in_bytes() const {
  size_t ret = 0;
  ret += util::size_in_bytes(chunks_);
  ret += sizeof(size_);
  ret += sizeof(bits_);
  ret += sizeof(mask_);
  return ret;
}

void SmallVector::write(std::ostream& os) const {
  util::write_vector(chunks_, os);
  util::write_value(size_, os);
  util::write_value(bits_, os);
  util::write_value(mask_, os);
}

void SmallVector::read(std::istream& is) {
  util::read_vector(chunks_, is);
  util::read_value(size_, is);
  util::read_value(bits_, is);
  util::read_value(mask_, is);
}

void SmallVector::swap(SmallVector& rhs) {
  chunks_.swap(rhs.chunks_);
  std::swap(size_, rhs.size_);
  std::swap(bits_, rhs.bits_);
  std::swap(mask_, rhs.mask_);
}

} //namespace - xcdat
