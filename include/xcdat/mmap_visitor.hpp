#pragma once

#include <type_traits>

#include "immutable_vector.hpp"

namespace xcdat {

class mmap_visitor {
  private:
    const char* m_base = nullptr;
    const char* m_cur = nullptr;

  public:
    mmap_visitor(const char* base) : m_base(base), m_cur(base) {}

    virtual ~mmap_visitor() = default;

    template <typename T>
    void visit(immutable_vector<T>& vec) {
        m_cur += vec.mmap(m_cur);
    }

    template <typename T>
    void visit(T& obj) {
        if constexpr (std::is_pod_v<T>) {
            obj = *reinterpret_cast<const T*>(m_cur);
            m_cur += sizeof(T);
        } else {
            obj.visit(*this);
        }
    }

    std::uint64_t bytes() {
        return std::distance(m_base, m_cur);
    }
};

}  // namespace xcdat
