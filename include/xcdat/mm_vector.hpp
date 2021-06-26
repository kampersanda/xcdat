#pragma once

#include <cstdlib>
#include <vector>

namespace xcdat {

//! A memory-mappable vector.
template <class T>
class mm_vector {
  private:
    std::vector<T> m_vec;

  public:
    mm_vector() = default;
    virtual ~mm_vector() = default;

    mm_vector(const mm_vector&) = delete;
    mm_vector& operator=(const mm_vector&) = delete;

    mm_vector(mm_vector&&) noexcept = default;
    mm_vector& operator=(mm_vector&&) noexcept = default;

    explicit mm_vector(std::vector<T>&& vec) {
        steal(vec);
    }

    void steal(std::vector<T>& vec) {
        m_vec.swap(vec);
        m_vec.shrink_to_fit();
    }

    void clear() {
        m_vec.clear();
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

    inline auto rbegin() const {
        return m_vec.rbegin();
    }

    inline auto rend() const {
        return m_vec.rend();
    }

    inline const T& operator[](std::uint64_t i) const {
        return m_vec[i];
    }

    inline const T* data() const {
        return m_vec.data();
    }

    template <class Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_vec);
    }
};

}  // namespace xcdat