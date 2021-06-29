#pragma once

#include <array>

#include "bit_vector.hpp"
#include "compact_vector.hpp"

namespace xcdat {

class bc_vector_8 {
  public:
    static constexpr std::uint32_t l1_bits = 8;
    static constexpr std::uint32_t max_levels = sizeof(std::uint64_t);

  private:
    std::uint32_t m_num_levels = 0;
    std::uint64_t m_num_frees = 0;
    std::array<immutable_vector<std::uint8_t>, max_levels> m_bytes;
    std::array<bit_vector, max_levels - 1> m_nexts;
    compact_vector m_links;
    bit_vector m_leaves;

  public:
    bc_vector_8() = default;
    virtual ~bc_vector_8() = default;

    bc_vector_8(const bc_vector_8&) = delete;
    bc_vector_8& operator=(const bc_vector_8&) = delete;

    bc_vector_8(bc_vector_8&&) noexcept = default;
    bc_vector_8& operator=(bc_vector_8&&) noexcept = default;

    template <class BcUnits>
    explicit bc_vector_8(const BcUnits& bc_units, bit_vector::builder&& leaves) {
        std::array<std::vector<std::uint8_t>, max_levels> bytes;
        std::array<bit_vector::builder, max_levels> next_flags;  // The last will not be released
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
            m_bytes[i].build(bytes[i]);
            m_nexts[i] = bit_vector(next_flags[i], true, false);
        }
        m_bytes[m_num_levels].build(bytes[m_num_levels]);
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
        return m_bytes[0][i * 2] | (m_links[m_leaves.rank(i)] << 8);
    }

    inline bool is_leaf(std::uint64_t i) const {
        return m_leaves[i];
    }

    inline bool is_used(std::uint64_t i) const {
        return check(i) != i;
    }

    inline std::uint64_t num_units() const {
        return m_bytes[0].size() / 2;
    }

    inline std::uint64_t num_leaves() const {
        return m_leaves.num_ones();
    }

    template <class Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_num_levels);
        visitor.visit(m_num_frees);
        for (std::uint32_t j = 0; j < m_bytes.size(); j++) {
            visitor.visit(m_bytes[j]);
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
        std::uint64_t x = m_bytes[j][i];
        while (j < m_num_levels and m_nexts[j][i]) {
            i = m_nexts[j++].rank(i);
            x |= static_cast<std::uint64_t>(m_bytes[j][i]) << (j * 8);
        }
        return x;
    }
};

}  // namespace xcdat