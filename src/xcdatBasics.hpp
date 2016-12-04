#ifndef XCDAT_BASICS_HPP_
#define XCDAT_BASICS_HPP_

#include <algorithm>
#include <array>
#include <fstream>
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <utility>
#include <vector>

namespace xcdat {

// For builder
struct BcElement {
  uint32_t base : 31;
  bool is_leaf : 1;
  uint32_t check : 31;
  bool is_used : 1;
};

inline void show_size(const char* str, double size, std::ostream& os) {
  os << str << "\t" << size << std::endl;
}

inline void show_size(const char* str, size_t size, std::ostream& os) {
  os << str << "\t" << size << std::endl;
}

inline void show_size_ratio(const char* str, size_t size, size_t denom, std::ostream& os) {
  os << str << "\t" << size << "\t" << (double) size / denom << std::endl;
}

template<typename T>
inline size_t size_vector(const std::vector<T>& vec) {
  static_assert(!std::is_same<T, bool>::value, "no support type");
  return vec.size() * sizeof(T) + sizeof(vec.size());
}

template<typename T>
inline void write_value(const T val, std::ostream& os) {
  os.write(reinterpret_cast<const char*>(&val), sizeof(val));
}

template<typename T>
inline void write_vector(const std::vector<T>& vec, std::ostream& os) {
  static_assert(!std::is_same<T, bool>::value, "no support type");
  auto size = vec.size();
  write_value(size, os);
  os.write(reinterpret_cast<const char*>(&vec[0]), sizeof(T) * size);
}

template<typename T>
inline void read_value(T& val, std::istream& is) {
  is.read(reinterpret_cast<char*>(&val), sizeof(val));
}

template<typename T>
inline void read_vector(std::vector<T>& vec, std::istream& is) {
  static_assert(!std::is_same<T, bool>::value, "no support type");
  vec.clear();
  size_t size = 0;
  read_value(size, is);
  vec.resize(size);
  is.read(reinterpret_cast<char*>(&vec[0]), sizeof(T) * size);
}

} //namespace - xcdat

#endif //XCDAT_BASICS_HPP_
