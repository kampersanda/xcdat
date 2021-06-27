#pragma once

#include <cstdlib>
#include <vector>

namespace xcdat {

template <class T>
class immutable_vector {
  private:
    std::vector<T> m_vec;

  public:
    immutable_vector() = default;
    virtual ~immutable_vector() = default;

    immutable_vector(const immutable_vector&) = delete;
    immutable_vector& operator=(const immutable_vector&) = delete;

    immutable_vector(immutable_vector&&) noexcept = default;
    immutable_vector& operator=(immutable_vector&&) noexcept = default;

    explicit immutable_vector(std::vector<T>&& vec) {
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
        *this = immutable_vector<T>();
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
