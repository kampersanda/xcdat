#ifndef XCDAT_EXCEPTION_HPP_
#define XCDAT_EXCEPTION_HPP_

#include <exception>

#include "xcdatBasics.hpp"

namespace xcdat {

class Exception : public std::exception {
public:
  Exception(const char* message, const char* file_name,
            const char* func_name, const int line)
    : message_{message}, file_name_{file_name}, func_name_{func_name}, line_{line} {}

  virtual ~Exception() throw() {}

  virtual const char* what() const throw() {
    return message_.c_str();
  }

  const char* file_name() const {
    return file_name_;
  }
  const char* func_name() const {
    return func_name_;
  }
  int line() const {
    return line_;
  }

private:
  std::string message_;
  const char* file_name_ = nullptr;
  const char* func_name_ = nullptr;
  int line_ = 0;
};

} //namespace - xcdat

#endif //XCDAT_EXCEPTION_HPP_
