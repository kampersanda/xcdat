#pragma once

#include "mm_vector.hpp"
#include "utils.hpp"

namespace xcdat {

class compact_vector {
  private:
    std::uint64_t m_size = 0;
    std::uint64_t m_bits = 0;
    std::uint64_t m_mask = 0;
    mm_vector<std::uint64_t> m_chunks;

  public:
    compact_vector() = default;

    template <class Vec>
    compact_vector(const Vec& vec) {
        build(vec);
    }

    virtual ~compact_vector() = default;

    template <class Vec>
    void build(const Vec& vec) {
        const std::uint64_t maxv = *std::max_element(vec.begin(), vec.end());

        m_size = vec.size();
        m_bits = utils::bits_for_int(maxv);
        m_mask = (1ULL << m_bits) - 1;

        std::vector<std::uint64_t> chunks(utils::words_for_bits(m_size * m_bits));

        for (std::uint64_t i = 0; i < m_size; i++) {
            const auto [quo, mod] = utils::decompose<64>(i * m_bits);
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
        const auto [quo, mod] = utils::decompose<64>(i * m_bits);
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

    inline std::uint64_t memory_in_bytes() const {
        return m_chunks.size() * sizeof(std::uint64_t);
    }
};

}  // namespace xcdat