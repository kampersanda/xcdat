//
// Created by Kampersanda on 2017/01/28.
//

#ifndef XCDAT_VECTOR_HPP
#define XCDAT_VECTOR_HPP

#include "xcdat_basics.hpp"

namespace xcdat {

// Simple vector
template<typename T>
class Vector {
public:
  static_assert(!std::is_same<T, bool>::value, "Type bool is not supported.");
  static_assert(std::is_pod<T>::value, "T is not POD.");

  Vector() = default;

  explicit Vector(std::istream& is) {
    size_ = read_value<size_t>(is);
    vec_.resize(size_);
    is.read(reinterpret_cast<char*>(&vec_[0]), sizeof(T) * size_);
    data_ = vec_.data();
  }

  explicit Vector(std::vector<T>& vec) {
    if (vec.size() != vec.capacity()) {
      vec.shrink_to_fit();
    }
    vec_ = std::move(vec);
    data_ = vec_.data();
    size_ = vec_.size();
  }

  ~Vector() = default;

  const T& operator[](size_t i) const {
    return data_[i];
  }
  const T* data() const {
    return data_;
  }

  const T* begin() const {
    return data_;
  }
  const T* end() const {
    return data_ + size_;
  }

  bool is_empty() const {
    return size_ == 0;
  }

  size_t size() const {
    return size_;
  }

  size_t size_in_bytes() const {
    return size_ * sizeof(T) + sizeof(size_);
  }

  void write(std::ostream& os) const {
    write_value(size_, os);
    os.write(reinterpret_cast<const char*>(data_), sizeof(T) * size_);
  }

  Vector(const Vector&) = delete;
  Vector& operator=(const Vector&) = delete;

  Vector(Vector&&) noexcept = default;
  Vector& operator=(Vector&&) noexcept = default;

private:
  const T* data_ {};
  size_t size_ {};
  std::vector<T> vec_ {};
};

}

#endif //XCDAT_VECTOR_HPP
