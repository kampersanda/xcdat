#ifndef XCDAT_CHAR_RANGE_HPP_
#define XCDAT_CHAR_RANGE_HPP_

#include "xcdatBasics.hpp"

namespace xcdat {

/*
 * A query string
 * */
struct CharRange {
  using Type = const uint8_t*;

  Type begin = nullptr;
  Type end = nullptr;

  CharRange() {}
  CharRange(const std::string& string)
    : CharRange{string.c_str(), string.c_str() + string.length()} {}
  CharRange(const char* b, const char* e)
    : begin{reinterpret_cast<Type>(b)}, end{reinterpret_cast<Type>(e)} {}
  CharRange(Type b, Type e) : begin{b}, end{e} {}

  size_t length() const { return static_cast<size_t>(end - begin); }
};

inline bool operator==(const CharRange& lhs, const CharRange& rhs) {
  if (lhs.length() != rhs.length()) {
    return false;
  }
  return std::equal(lhs.begin, lhs.end, rhs.begin);
}

inline bool operator!=(const CharRange& lhs, const CharRange& rhs) {
  return !(lhs == rhs);
}

inline bool operator<(const CharRange& lhs, const CharRange& rhs) {
  return std::lexicographical_compare(lhs.begin, lhs.end, rhs.begin, rhs.end);
}

inline std::ostream& operator<<(std::ostream& os, const CharRange& string) {
  for (auto it = string.begin; it != string.end; ++it) {
    os << char(*it);
  }
  return os;
}

} //namespace - xcdat

#endif //XCDAT_CHAR_RANGE_HPP_
