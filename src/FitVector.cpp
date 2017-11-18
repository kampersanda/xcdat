#include "xcdat/FitVector.hpp"

namespace xcdat {

FitVector::FitVector(std::istream& is) {
  chunks_ = Vector<id_type>(is);
  size_= read_value<size_t>(is);
  width_ = read_value<id_type>(is);
  mask_ = read_value<id_type>(is);
}

FitVector::FitVector(const std::vector<id_type>& values) {
  if (values.empty()) {
    return;
  }

  width_ = 0;
  auto max_value = *std::max_element(std::begin(values), std::end(values));
  do {
    ++width_;
    max_value >>= 1;
  } while (max_value);

  size_ = values.size();
  mask_ = (1U << width_) - 1;
  std::vector<id_type> chunks(size_ * width_ / CHUNK_WIDTH + 1, 0);

  for (id_type i = 0; i < size_; ++i) {
    const auto chunk_pos = static_cast<id_type>(i * width_ / CHUNK_WIDTH);
    const auto offset = static_cast<id_type>(i * width_ % CHUNK_WIDTH);

    chunks[chunk_pos] &= ~(mask_ << offset);
    chunks[chunk_pos] |= (values[i] & mask_) << offset;

    if (CHUNK_WIDTH < offset + width_) {
      chunks[chunk_pos + 1] &= ~(mask_ >> (CHUNK_WIDTH - offset));
      chunks[chunk_pos + 1] |= (values[i] & mask_) >> (CHUNK_WIDTH - offset);
    }
  }

  chunks_ = Vector<id_type>(chunks);
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

} //namespace - xcdat
