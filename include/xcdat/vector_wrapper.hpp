#pragma once

#include <cstdlib>
#include <vector>

namespace xcdat {

template <class T>
class vector_wrapper {
  private:
    std::vector<T> m_vec;

  public:
    vector_wrapper() = default;
    virtual ~vector_wrapper() = default;

    vector_wrapper(const vector_wrapper&) = delete;
    vector_wrapper& operator=(const vector_wrapper&) = delete;

    vector_wrapper(vector_wrapper&&) noexcept = default;
    vector_wrapper& operator=(vector_wrapper&&) noexcept = default;

    explicit vector_wrapper(std::vector<T>&& vec) {
        steal(vec);
    }

    void steal(std::vector<T>& vec) {
        if (vec.size() != 0) {
            m_vec = std::move(vec);
            m_vec.shrink_to_fit();
        } else {
            clear();
        }
    }

    void clear() {
        *this = vector_wrapper<T>();
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
