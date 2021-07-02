#pragma once

#include <exception>

namespace xcdat {

class exception : public std::exception {
  public:
    explicit exception(const char* msg) : msg_{msg} {}
    ~exception() throw() override = default;

    const char* what() const throw() override {
        return msg_;
    }

  private:
    const char* msg_;
};

#define XCDAT_TO_STR_(n) #n
#define XCDAT_TO_STR(n) XCDAT_TO_STR_(n)
#define XCDAT_THROW(msg) throw xcdat::exception(__FILE__ ":" XCDAT_TO_STR(__LINE__) ":" msg)
#define XCDAT_THROW_IF(cond, msg) (void)((!(cond)) || (XCDAT_THROW(msg), 0))

}  // namespace xcdat
