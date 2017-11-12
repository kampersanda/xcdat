#ifndef XCDAT_BASICS_HPP_
#define XCDAT_BASICS_HPP_

#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <utility>
#include <vector>
#include <limits>

#include "xcdat_config.hpp"

namespace xcdat {

#ifdef XCDAT_X64
using id_type = uint64_t;
#else
using id_type = uint32_t;
#endif

constexpr id_type ID_MAX = std::numeric_limits<id_type>::max();

struct BcPair {
  id_type base;
  id_type check;
};

inline void show_size(const char* str, double size, std::ostream& os) {
  os << str << "\t" << size << std::endl;
}

inline void show_size(const char* str, size_t size, std::ostream& os) {
  os << str << "\t" << size << std::endl;
}

inline void show_size_ratio(const char* str, size_t size, size_t denom, std::ostream& os) {
  os << str << "\t" << size << "\t" << static_cast<double>(size) / denom << std::endl;
}

template<typename T>
inline void write_value(const T val, std::ostream& os) {
  os.write(reinterpret_cast<const char*>(&val), sizeof(val));
}

template<typename T>
inline T read_value(std::istream& is) {
  T val;
  is.read(reinterpret_cast<char*>(&val), sizeof(val));
  return val;
}

} //namespace - xcdat

#endif //XCDAT_BASICS_HPP_
