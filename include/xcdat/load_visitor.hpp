#pragma once

#include <string>
#include <type_traits>

#include "exception.hpp"
#include "immutable_vector.hpp"

namespace xcdat {

class load_visitor {
  private:
    std::ifstream m_ifs;

  public:
    load_visitor(const std::string& filepath) : m_ifs(filepath, std::ios::binary) {
        XCDAT_THROW_IF(!m_ifs.good(), "Cannot open the input file");
    }

    virtual ~load_visitor() {
        m_ifs.close();
    }

    template <class T>
    void visit(immutable_vector<T>& vec) {
        vec.load(m_ifs);
    }

    template <class T>
    void visit(T& obj) {
        if constexpr (std::is_pod_v<T>) {
            m_ifs.read(reinterpret_cast<char*>(&obj), sizeof(T));
        } else {
            obj.visit(*this);
        }
    }

    std::uint64_t bytes() {
        return m_ifs.tellg();
    }
};

}  // namespace xcdat
