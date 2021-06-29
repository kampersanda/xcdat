#pragma once

#include <string_view>
#include <type_traits>

#include "exception.hpp"
#include "immutable_vector.hpp"

namespace xcdat {

class size_visitor {
  private:
    std::uint64_t m_bytes = 0;

  public:
    size_visitor() = default;

    virtual ~size_visitor() = default;

    template <typename T>
    void visit(const immutable_vector<T>& vec) {
        m_bytes += vec.memory_in_bytes();
    }

    template <typename T>
    void visit(const T& obj) {
        if constexpr (std::is_pod_v<T>) {
            m_bytes += sizeof(T);
        } else {
            const_cast<T&>(obj).visit(*this);
        }
    }

    std::uint64_t bytes() {
        return m_bytes;
    }
};

}  // namespace xcdat
