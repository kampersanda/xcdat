//
// Created by Kampersanda on 2017/01/28.
//

#ifndef XCDAT_VECTOR_HPP
#define XCDAT_VECTOR_HPP

#include "xcdatBasics.hpp"

namespace xcdat {

/*
 * Simple vector
 * */
template<typename T>
class Vector {
public:
  Vector() {
    static_assert(!Is_same<T, bool>(), "Type bool is not supported.");
    static_assert(Is_pod<T>(), "T is not POD.");
  }

  ~Vector() {}

  void steal(std::vector<T>& vec) {
    Vector().swap(*this);
    if (vec.size() != vec.capacity()) {
      vec.shrink_to_fit();
    }
    buf_.swap(vec);
    data_ = buf_.data();
    size_ = buf_.size();
  }

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

  void read(std::istream& is) {
    Vector().swap(*this);
    read_value(size_, is);
    buf_.resize(size_);
    is.read(reinterpret_cast<char*>(&buf_[0]), sizeof(T) * size_);
    data_ = buf_.data();
  }

  void swap(Vector<T>& rhs) {
    std::swap(data_, rhs.data_);
    std::swap(size_, rhs.size_);
    buf_.swap(rhs.buf_);
  }

  Vector(const Vector&) = delete;
  Vector& operator=(const Vector&) = delete;

private:
  const T* data_ = nullptr;
  size_t size_ = 0;
  std::vector<T> buf_;
};

}

#endif //XCDAT_VECTOR_HPP
