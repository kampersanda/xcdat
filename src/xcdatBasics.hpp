#ifndef XCDAT_BASICS_HPP_
#define XCDAT_BASICS_HPP_

#include <algorithm>
#include <exception>
#include <fstream>
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <utility>
#include <vector>

namespace xcdat {

constexpr auto kNotFound = static_cast<uint32_t>(-1);

// For Base and Check arrays
constexpr uint32_t kRootId = 0;
constexpr uint32_t kTabooId = 1;
constexpr uint32_t kBcUpper = (1U << 31) - 1;

// For builder
struct BcItem {
  uint32_t base : 31;
  bool is_leaf : 1;
  uint32_t check : 31;
  bool is_used : 1;
};

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

#define XCDAT_THROW(message) \
  throw Exception(message, __FILE__, __func__, __LINE__)

class Exception : public std::exception {
public:
  Exception(const char* message, const char* file_name,
            const char* func_name, const int line)
    : message_{message}, file_name_{file_name}, func_name_{func_name}, line_{line} {}
  virtual ~Exception() throw() {}

  virtual const char* what() const throw() override {
    return message_.c_str();
  }

  const char* file_name() const { return file_name_; }
  const char* func_name() const { return func_name_; }
  int line() const { return line_; }

private:
  std::string message_;
  const char* file_name_ = nullptr;
  const char* func_name_ = nullptr;
  int line_ = 0;
};

namespace util {

template<typename T>
inline size_t size_in_bytes(const std::vector<T>& vec) {
  static_assert(!std::is_same<T, bool>::value, "no support type");
  return vec.size() * sizeof(T) + sizeof(vec.size());
}

inline void show_stat(const char* str, double size, std::ostream& os) {
  os << str << "\t" << size << std::endl;
}

inline void show_stat(const char* str, size_t size, std::ostream& os) {
  os << str << "\t" << size << std::endl;
}

inline void show_stat(const char* str, size_t size, size_t denom, std::ostream& os) {
  os << str << "\t" << size << "\t" << (double) size / denom << std::endl;
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

} //namespace - util

} //namespace - xcdat

#endif //XCDAT_BASICS_HPP_
