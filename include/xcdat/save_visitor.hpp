#pragma once

#include <string>
#include <type_traits>

#include "exception.hpp"
#include "immutable_vector.hpp"

namespace xcdat {

class save_visitor {
  private:
    std::ofstream m_ofs;

  public:
    save_visitor(const std::string& filepath) : m_ofs(filepath, std::ios::binary) {
        XCDAT_THROW_IF(!m_ofs.good(), "Cannot open the input file");
    }

    virtual ~save_visitor() {
        m_ofs.close();
    }

    template <typename T>
    void visit(const immutable_vector<T>& vec) {
        vec.save(m_ofs);
    }

    template <typename T>
    void visit(const T& obj) {
        if constexpr (std::is_pod_v<T>) {
            m_ofs.write(reinterpret_cast<const char*>(&obj), sizeof(T));
        } else {
            const_cast<T&>(obj).visit(*this);
        }
    }

    std::uint64_t bytes() {
        return m_ofs.tellp();
    }
};

}  // namespace xcdat