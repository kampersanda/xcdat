#pragma once

#include <cstdlib>
#include <vector>

namespace xcdat {

template <class T>
class mm_vector {
  private:
    std::vector<T> m_vec;

  public:
    mm_vector() = default;

    virtual ~mm_vector() = default;

    // NOTE: The input vector is stolen.
    mm_vector(std::vector<T>& vec) {
        steal(vec);
    }

    void steal(std::vector<T>& vec) {
        m_vec.swap(vec);
        m_vec.shrink_to_fit();
    }

    void reset() {
        m_vec = std::vector<T>();
    }

    inline std::uint64_t size() const {
        return m_vec.size();
    }

    inline auto begin() const {
        return m_vec.begin();
    }

    inline auto end() const {
        return m_vec.end();
    }

    inline const T& operator[](std::uint64_t i) const {
        return m_vec[i];
    }

    inline const T* data() const {
        return m_vec.data();
    }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_vec);
    }
};

}  // namespace xcdat