#pragma once

#include "essentials/essentials.hpp"

#include "bit_tools.hpp"
#include "exception.hpp"
#include "vector_wrapper.hpp"

namespace xcdat {

//! A compressed integer vector.
class compact_vector {
  private:
    std::uint64_t m_size = 0;
    std::uint64_t m_bits = 0;
    std::uint64_t m_mask = 0;
    vector_wrapper<std::uint64_t> m_chunks;

  public:
    compact_vector() = default;
    virtual ~compact_vector() = default;

    compact_vector(const compact_vector&) = delete;
    compact_vector& operator=(const compact_vector&) = delete;

    compact_vector(compact_vector&&) noexcept = default;
    compact_vector& operator=(compact_vector&&) noexcept = default;

    template <class Vec>
    explicit compact_vector(const Vec& vec) {
        build(vec);
    }

    template <class Vec>
    void build(const Vec& vec) {
        XCDAT_THROW_IF(vec.size() == 0, "The input vector is empty.");

        const std::uint64_t maxv = *std::max_element(vec.begin(), vec.end());

        m_size = vec.size();
        m_bits = needed_bits(maxv);
        m_mask = (1ULL << m_bits) - 1;

        std::vector<std::uint64_t> chunks(essentials::words_for(m_size * m_bits));

        for (std::uint64_t i = 0; i < m_size; i++) {
            const auto [quo, mod] = decompose(i * m_bits);
            chunks[quo] &= ~(m_mask << mod);
            chunks[quo] |= (vec[i] & m_mask) << mod;
            if (64 < mod + m_bits) {
                const std::uint64_t diff = 64ULL - mod;
                chunks[quo + 1] &= ~(m_mask >> diff);
                chunks[quo + 1] |= (vec[i] & m_mask) >> diff;
            }
        }
        m_chunks.steal(chunks);
    }

    inline std::uint64_t operator[](std::uint64_t i) const {
        assert(i < m_size);
        const auto [quo, mod] = decompose(i * m_bits);
        if (mod + m_bits <= 64) {
            return (m_chunks[quo] >> mod) & m_mask;
        } else {
            return ((m_chunks[quo] >> mod) | (m_chunks[quo + 1] << (64 - mod))) & m_mask;
        }
    }

    inline std::uint64_t size() const {
        return m_size;
    }

    inline std::uint64_t bits() const {
        return m_bits;
    }

    template <class Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_size);
        visitor.visit(m_bits);
        visitor.visit(m_mask);
        visitor.visit(m_chunks);
    }

  private:
    static std::uint64_t needed_bits(std::uint64_t x) {
        return bit_tools::msb(x) + 1;
    }

    static std::tuple<std::uint64_t, std::uint64_t> decompose(std::uint64_t x) {
        return {x / 64, x % 64};
    }
};

}  // namespace xcdat