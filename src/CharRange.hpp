#ifndef XCDAT_CHAR_RANGE_HPP_
#define XCDAT_CHAR_RANGE_HPP_

#include "xcdatBasics.hpp"

namespace xcdat {

/*
 * Character range for handling a query string.
 * */
struct CharRange {
  using Type = const uint8_t*;

  // pointers indicating begin and end positions of the string.
  Type begin = nullptr;
  Type end = nullptr;

  /*
   * Generic constructor.
   * */
  CharRange() {}

  /*
   * Adopts a query string.
   * */
  CharRange(Type b, Type e) : begin{b}, end{e} {}
  CharRange(const char* b, const char* e)
    : begin{reinterpret_cast<Type>(b)}, end{reinterpret_cast<Type>(e)} {}
  CharRange(const std::string& string)
    : CharRange{string.c_str(), string.c_str() + string.length()} {}

  /*
   * Gets the distance between the range, that is, the string length.
   *  @returns the string length.
   * */
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
