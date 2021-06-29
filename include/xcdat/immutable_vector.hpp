#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>

namespace xcdat {

template <class T>
class immutable_vector {
  private:
    std::unique_ptr<T[]> m_allocator;
    std::uint64_t m_size = 0;
    const T* m_data = nullptr;

  public:
    immutable_vector() = default;
    virtual ~immutable_vector() = default;

    immutable_vector(const immutable_vector&) = delete;
    immutable_vector& operator=(const immutable_vector&) = delete;

    immutable_vector(immutable_vector&&) noexcept = default;
    immutable_vector& operator=(immutable_vector&&) noexcept = default;

    void clear() {
        m_allocator.reset();
        m_size = 0;
        m_data = nullptr;
    }

    template <class Vector>
    immutable_vector(const Vector& vec) {
        build(vec);
    }

    template <class Vector>
    void build(const Vector& vec) {
        clear();
        if (vec.size() != 0) {
            m_allocator = std::make_unique<T[]>(vec.size());
            std::copy_n(vec.data(), vec.size(), m_allocator.get());
            m_size = vec.size();
            m_data = m_allocator.get();
        }
    }

    std::uint64_t mmap(const char* address) {
        clear();
        m_size = *reinterpret_cast<const std::uint64_t*>(address);
        m_data = reinterpret_cast<const T*>(address + sizeof(std::uint64_t));
        return sizeof(std::uint64_t) + m_size * sizeof(T);
    }

    void load(std::ifstream& ifs) {
        clear();
        ifs.read(reinterpret_cast<char*>(&m_size), sizeof(m_size));
        if (m_size != 0) {
            m_allocator = std::make_unique<T[]>(m_size);
            ifs.read(reinterpret_cast<char*>(m_allocator.get()), sizeof(T) * m_size);
            m_data = m_allocator.get();
        }
    }

    void save(std::ofstream& ofs) const {
        ofs.write(reinterpret_cast<const char*>(&m_size), sizeof(m_size));
        ofs.write(reinterpret_cast<const char*>(m_data), sizeof(T) * m_size);
    }

    inline std::uint64_t memory_in_bytes() const {
        return sizeof(m_size) + sizeof(T) * m_size;
    }

    inline std::uint64_t size() const {
        return m_size;
    }

    inline const T* begin() const {
        return m_data;
    }

    inline const T* end() const {
        return m_data + m_size;
    }

    inline auto rbegin() const {
        return std::make_reverse_iterator(end());
    }

    inline auto rend() const {
        return std::make_reverse_iterator(begin());
    }

    inline const T& operator[](std::uint64_t i) const {
        assert(i < m_size);
        return m_data[i];
    }

    inline const T* data() const {
        return m_data;
    }
};

}  // namespace xcdat
