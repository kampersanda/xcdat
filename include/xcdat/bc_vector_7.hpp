#pragma once

#include <array>

#include "bit_vector.hpp"
#include "compact_vector.hpp"

namespace xcdat {

class bc_vector_7 {
  public:
    static constexpr std::uint32_t l1_bits = 7;
    static constexpr std::uint32_t max_levels = 4;

    static constexpr std::uint64_t block_size_l1 = 1ULL << 7;
    static constexpr std::uint64_t block_size_l2 = 1ULL << 15;
    static constexpr std::uint64_t block_size_l3 = 1ULL << 31;

  private:
    std::uint64_t m_num_frees = 0;
    immutable_vector<std::uint8_t> m_ints_l1;
    immutable_vector<std::uint16_t> m_ints_l2;
    immutable_vector<std::uint32_t> m_ints_l3;
    immutable_vector<std::uint64_t> m_ints_l4;
    std::array<immutable_vector<std::uint64_t>, max_levels - 1> m_ranks;
    compact_vector m_links;
    bit_vector m_leaves;

  public:
    bc_vector_7() = default;
    virtual ~bc_vector_7() = default;

    bc_vector_7(const bc_vector_7&) = delete;
    bc_vector_7& operator=(const bc_vector_7&) = delete;

    bc_vector_7(bc_vector_7&&) noexcept = default;
    bc_vector_7& operator=(bc_vector_7&&) noexcept = default;

    template <class BcUnits>
    explicit bc_vector_7(const BcUnits& bc_units, bit_vector::builder&& leaves) {
        std::vector<std::uint8_t> ints_l1;
        std::vector<std::uint16_t> ints_l2;
        std::vector<std::uint32_t> ints_l3;
        std::vector<std::uint64_t> ints_l4;
        std::array<std::vector<std::uint64_t>, max_levels - 1> ranks;
        std::vector<std::uint64_t> links;

        ints_l1.reserve(bc_units.size() * 2);
        ranks[0].reserve((bc_units.size() * 2) >> l1_bits);
        links.reserve(bc_units.size());

        auto append_unit = [&](std::uint64_t x) {
            if ((ints_l1.size() % block_size_l1) == 0) {
                ranks[0].push_back(static_cast<std::uint64_t>(ints_l2.size()));
            }
            if ((x / block_size_l1) == 0) {
                ints_l1.push_back(static_cast<std::uint8_t>(0 | (x << 1)));
                return;
            } else {
                const auto i = ints_l2.size() - ranks[0].back();
                ints_l1.push_back(static_cast<std::uint8_t>(1 | (i << 1)));
            }

            if ((ints_l2.size() % block_size_l2) == 0) {
                ranks[1].push_back(static_cast<std::uint64_t>(ints_l3.size()));
            }
            if ((x / block_size_l2) == 0) {
                ints_l2.push_back(static_cast<std::uint16_t>(0 | (x << 1)));
                return;
            } else {
                const auto i = ints_l3.size() - ranks[1].back();
                ints_l2.push_back(static_cast<std::uint16_t>(1 | (i << 1)));
            }

            if ((ints_l3.size() % block_size_l3) == 0) {
                ranks[2].push_back(static_cast<std::uint64_t>(ints_l4.size()));
            }
            if ((x / block_size_l3) == 0) {
                ints_l3.push_back(static_cast<std::uint32_t>(0 | (x << 1)));
                return;
            } else {
                const auto i = ints_l4.size() - ranks[2].back();
                ints_l3.push_back(static_cast<std::uint32_t>(1 | (i << 1)));
            }
            ints_l4.push_back(x);
        };

        auto append_leaf = [&](std::uint64_t x) {
            if ((ints_l1.size() % block_size_l1) == 0) {
                ranks[0].push_back(static_cast<std::uint64_t>(ints_l2.size()));
            }
            ints_l1.push_back(static_cast<std::uint8_t>(x & 0xFF));
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
        m_ints_l1.build(ints_l1);
        m_ints_l2.build(ints_l2);
        m_ints_l3.build(ints_l3);
        m_ints_l4.build(ints_l4);
        for (std::uint32_t j = 0; j < m_ranks.size(); ++j) {
            m_ranks[j].build(ranks[j]);
        }
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
        return m_ints_l1[i * 2] | (m_links[m_leaves.rank(i)] << 8);
    }

    inline bool is_leaf(std::uint64_t i) const {
        return m_leaves[i];
    }

    inline bool is_used(std::uint64_t i) const {
        return check(i) != i;
    }

    inline std::uint64_t num_units() const {
        return m_ints_l1.size() / 2;
    }

    inline std::uint64_t num_leaves() const {
        return m_leaves.num_ones();
    }

    template <class Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_num_frees);
        visitor.visit(m_ints_l1);
        visitor.visit(m_ints_l2);
        visitor.visit(m_ints_l3);
        visitor.visit(m_ints_l4);
        for (std::uint32_t j = 0; j < m_ranks.size(); j++) {
            visitor.visit(m_ranks[j]);
        }
        visitor.visit(m_links);
        visitor.visit(m_leaves);
    }

  private:
    inline std::uint64_t access(std::uint64_t i) const {
        std::uint64_t x = m_ints_l1[i] >> 1;
        if ((m_ints_l1[i] & 1U) == 0) {
            return x;
        }
        i = m_ranks[0][i / block_size_l1] + x;

        x = m_ints_l2[i] >> 1;
        if ((m_ints_l2[i] & 1U) == 0) {
            return x;
        }
        i = m_ranks[1][i / block_size_l2] + x;

        x = m_ints_l3[i] >> 1;
        if ((m_ints_l3[i] & 1U) == 0) {
            return x;
        }
        i = m_ranks[2][i / block_size_l3] + x;

        return m_ints_l4[i];
    }
};

}  // namespace xcdat