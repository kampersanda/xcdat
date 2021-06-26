#pragma once

#include <array>

#include "bit_vector.hpp"
#include "compact_vector.hpp"

namespace xcdat {

class bc_vector {
  public:
    static constexpr std::uint32_t l1_bits = 8;
    static constexpr std::uint32_t max_levels = sizeof(std::uint64_t);

  private:
    std::uint32_t m_num_levels = 0;
    std::uint64_t m_num_frees = 0;
    std::array<mm_vector<std::uint8_t>, max_levels> m_bytes;
    std::array<bit_vector, max_levels - 1> m_next_flags;
    compact_vector m_links;
    bit_vector m_leaf_flags;

  public:
    bc_vector() = default;

    template <class BcUnit>
    bc_vector(const std::vector<BcUnit>& bc_units, bit_vector::builder& leaf_flags)
        : bc_vector(bc_units, std::move(leaf_flags)) {}

    template <class BcUnit>
    bc_vector(const std::vector<BcUnit>& bc_units, bit_vector::builder&& leaf_flags) {
        std::array<std::vector<std::uint8_t>, max_levels> bytes;
        std::array<bit_vector::builder, max_levels - 1> next_flags;
        std::vector<std::uint64_t> links;

        bytes[0].reserve(bc_units.size() * 2);
        next_flags[0].reserve(bc_units.size() * 2);
        links.reserve(bc_units.size());

        m_num_levels = 0;

        auto append_unit = [&](std::uint64_t x) {
            std::uint32_t j = 0;
            bytes[j].push_back(static_cast<std::uint8_t>(x & 0xFF));
            next_flags[j].push_back(true);
            x >>= 8;
            while (x) {
                ++j;
                bytes[j].push_back(static_cast<std::uint8_t>(x & 0xFF));
                next_flags[j].push_back(true);
                x >>= 8;
            }
            next_flags[j].set_bit(next_flags[j].size() - 1, false);
            m_num_levels = std::max(m_num_levels, j);
        };

        auto append_leaf = [&](std::uint64_t x) {
            bytes[0].push_back(static_cast<std::uint8_t>(x & 0xFF));
            next_flags[0].push_back(false);
            links.push_back(x >> 8);
        };

        for (std::uint64_t i = 0; i < bc_units.size(); ++i) {
            if (leaf_flags[i]) {
                append_leaf(bc_units[i].base);
            } else {
                append_unit(bc_units[i].base ^ i);
            }
            append_unit(bc_units[i].check ^ i);
            if (bc_units[i].check == i) {
                m_num_frees += 1;
            }
        }

        // release
        for (uint8_t i = 0; i < m_num_levels; ++i) {
            m_bytes[i].steal(bytes[i]);
            m_next_flags[i].build(next_flags[i], true, false);
        }
        m_bytes[m_num_levels].steal(bytes[m_num_levels]);
        m_links.build(links);
        m_leaf_flags.build(leaf_flags, true, false);
    }

    virtual ~bc_vector() = default;

    inline std::uint64_t base(std::uint64_t i) const {
        return access(i * 2) ^ i;
    }

    inline std::uint64_t check(std::uint64_t i) const {
        return access(i * 2 + 1) ^ i;
    }

    inline std::uint64_t link(std::uint64_t i) const {
        return m_bytes[0][i * 2] | (m_links[m_leaf_flags.rank(i)] << 8);
    }

    inline bool is_leaf(std::uint64_t i) const {
        return m_leaf_flags[i];
    }

    inline bool is_used(std::uint64_t i) const {
        return check(i) != i;
    }

    inline std::uint64_t num_units() const {
        return m_bytes[0].size() / 2;
    }

    inline std::uint64_t num_leaves() const {
        return m_leaf_flags.num_ones();
    }

  private:
    inline std::uint64_t access(std::uint64_t i) const {
        std::uint32_t j = 0;
        std::uint64_t x = m_bytes[j][i];
        while (j < m_num_levels and m_next_flags[j][i]) {
            i = m_next_flags[j++].rank(i);
            x |= static_cast<std::uint64_t>(m_bytes[j][i]) << (j * 8);
        }
        return x;
    }
};

}  // namespace xcdat