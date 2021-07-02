#pragma once

#include <array>

#include "bit_vector.hpp"
#include "compact_vector.hpp"

namespace xcdat {

class bc_vector_16 {
  public:
    static constexpr std::uint32_t l1_bits = sizeof(std::uint16_t) * 8;
    static constexpr std::uint32_t max_levels = sizeof(std::uint64_t) / sizeof(std::uint16_t);

  private:
    std::uint32_t m_num_levels = 0;
    std::uint64_t m_num_frees = 0;
    std::array<immutable_vector<std::uint16_t>, max_levels> m_shorts;
    std::array<bit_vector, max_levels - 1> m_nexts;
    compact_vector m_links;
    bit_vector m_leaves;

  public:
    bc_vector_16() = default;
    virtual ~bc_vector_16() = default;

    bc_vector_16(const bc_vector_16&) = delete;
    bc_vector_16& operator=(const bc_vector_16&) = delete;

    bc_vector_16(bc_vector_16&&) noexcept = default;
    bc_vector_16& operator=(bc_vector_16&&) noexcept = default;

    template <class BcUnits>
    explicit bc_vector_16(const BcUnits& bc_units, bit_vector::builder&& leaves) {
        std::array<std::vector<std::uint16_t>, max_levels> shorts;
        std::array<bit_vector::builder, max_levels> next_flags;  // The last will not be released
        std::vector<std::uint64_t> links;

        shorts[0].reserve(bc_units.size() * 2);
        next_flags[0].reserve(bc_units.size() * 2);
        links.reserve(bc_units.size());

        m_num_levels = 0;

        auto append_unit = [&](std::uint64_t x) {
            std::uint32_t j = 0;
            shorts[j].push_back(static_cast<std::uint16_t>(x & 0xFFFFU));
            next_flags[j].push_back(true);
            x >>= 16;
            while (x) {
                ++j;
                shorts[j].push_back(static_cast<std::uint16_t>(x & 0xFFFFU));
                next_flags[j].push_back(true);
                x >>= 16;
            }
            next_flags[j].set_bit(next_flags[j].size() - 1, false);
            m_num_levels = std::max(m_num_levels, j);
        };

        auto append_leaf = [&](std::uint64_t x) {
            shorts[0].push_back(static_cast<std::uint16_t>(x & 0xFFFFU));
            next_flags[0].push_back(false);
            links.push_back(x >> 16);
        };

        for (std::uint64_t i = 0; i < bc_units.size(); ++i) {
            if (leaves[i]) {
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
        for (std::uint32_t i = 0; i < m_num_levels; ++i) {
            m_shorts[i].build(shorts[i]);
            m_nexts[i] = bit_vector(next_flags[i], true, false);
        }
        m_shorts[m_num_levels].build(shorts[m_num_levels]);
        m_links = compact_vector(links);
        m_leaves = bit_vector(leaves, true, false);
    }

    inline std::uint64_t base(std::uint64_t i) const {
        return access(i * 2) ^ i;
    }

    inline std::uint64_t check(std::uint64_t i) const {
        return access(i * 2 + 1) ^ i;
    }

    inline std::uint64_t link(std::uint64_t i) const {
        return m_shorts[0][i * 2] | (m_links[m_leaves.rank(i)] << 16);
    }

    inline bool is_leaf(std::uint64_t i) const {
        return m_leaves[i];
    }

    inline bool is_used(std::uint64_t i) const {
        return check(i) != i;
    }

    inline std::uint64_t num_units() const {
        return m_shorts[0].size() / 2;
    }

    inline std::uint64_t num_free_units() const {
        return m_num_frees;
    }

    inline std::uint64_t num_nodes() const {
        return num_units() - num_free_units();
    }

    inline std::uint64_t num_leaves() const {
        return m_leaves.num_ones();
    }

    template <class Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_num_levels);
        visitor.visit(m_num_frees);
        for (std::uint32_t j = 0; j < m_shorts.size(); j++) {
            visitor.visit(m_shorts[j]);
        }
        for (std::uint32_t j = 0; j < m_nexts.size(); j++) {
            visitor.visit(m_nexts[j]);
        }
        visitor.visit(m_links);
        visitor.visit(m_leaves);
    }

  private:
    inline std::uint64_t access(std::uint64_t i) const {
        std::uint32_t j = 0;
        std::uint64_t x = m_shorts[j][i];
        while (j < m_num_levels and m_nexts[j][i]) {
            i = m_nexts[j++].rank(i);
            x |= static_cast<std::uint64_t>(m_shorts[j][i]) << (j * 16);
        }
        return x;
    }
};

}  // namespace xcdat