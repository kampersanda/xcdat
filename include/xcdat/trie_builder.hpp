#pragma once

#include <algorithm>
#include <iostream>
#include <string_view>

// #include "bc_vector.hpp"
#include "code_table.hpp"
#include "exception.hpp"
#include "tail_vector.hpp"

namespace xcdat {

template <class Strings>
class trie_builder {
    template <class>
    friend class trie;

  public:
    struct unit_type {
        std::uint64_t base;
        std::uint64_t check;
    };

  private:
    static constexpr std::uint64_t taboo_npos = 1;
    static constexpr std::uint64_t free_blocks = 16;

    const Strings& m_keys;
    const std::uint32_t m_l1_bits;  // # of bits for L1 layer of DACs
    const std::uint64_t m_l1_size;

    bool m_bin_mode = false;

    code_table m_table;
    std::vector<unit_type> m_units;
    bit_vector::builder m_leaves;
    bit_vector::builder m_terms;
    bit_vector::builder m_useds;
    std::vector<std::uint64_t> m_heads;  // for L1 blocks
    std::vector<std::uint8_t> m_edges;
    tail_vector::builder m_suffixes;

  public:
    explicit trie_builder(const Strings& keys, std::uint32_t l1_bits, bool bin_mode)
        : m_keys(keys), m_l1_bits(std::min(l1_bits, 8U)), m_l1_size(1ULL << m_l1_bits), m_bin_mode(bin_mode) {
        XCDAT_THROW_IF(m_keys.size() == 0, "The input dataset is empty.");

        // Reserve
        {
            std::uint64_t init_capa = 1;
            while (init_capa < m_keys.size()) {
                init_capa <<= 1;
            }
            m_units.reserve(init_capa);
            m_leaves.reserve(init_capa);
            m_terms.reserve(init_capa);
            m_useds.reserve(init_capa);
            m_heads.reserve(init_capa >> m_l1_bits);
            m_edges.reserve(256);
        }

        // Initialize an empty list.
        for (std::uint64_t npos = 0; npos < 256; ++npos) {
            m_units.push_back(unit_type{npos + 1, npos - 1});
            m_leaves.push_back(false);
            m_terms.push_back(false);
            m_useds.push_back(false);
        }
        m_units[255].base = 0;
        m_units[0].check = 255;

        for (std::uint64_t npos = 0; npos < 256; npos += m_l1_size) {
            m_heads.push_back(npos);
        }

        // Fix the root
        use_unit(0);
        m_units[0].check = taboo_npos;
        m_useds.set_bit(taboo_npos, true);
        m_heads[taboo_npos >> m_l1_bits] = m_units[taboo_npos].base;

        // Build the code table
        m_table = code_table(keys);
        m_bin_mode |= m_table.has_null();

        // Build the BC units
        arrange(0, m_keys.size(), 0, 0);

        // Build the TAIL vector
        m_suffixes.complete(m_bin_mode, [&](std::uint64_t npos, std::uint64_t tpos) { m_units[npos].base = tpos; });
    }

    virtual ~trie_builder() = default;

    trie_builder(const trie_builder&) = delete;
    trie_builder& operator=(const trie_builder&) = delete;

    trie_builder(trie_builder&&) noexcept = default;
    trie_builder& operator=(trie_builder&&) noexcept = default;

  private:
    inline void use_unit(std::uint64_t npos) {
        m_useds.set_bit(npos);

        const auto next = m_units[npos].base;
        const auto prev = m_units[npos].check;
        m_units[prev].base = next;
        m_units[next].check = prev;

        const auto lpos = npos >> m_l1_bits;
        if (m_heads[lpos] == npos) {
            m_heads[lpos] = (lpos != next >> m_l1_bits) ? taboo_npos : next;
        }
    }

    inline void close_block(std::uint64_t bpos) {
        const auto beg_npos = bpos * 256;
        const auto end_npos = beg_npos + 256;

        for (auto npos = beg_npos; npos < end_npos; ++npos) {
            if (!m_useds[npos]) {
                use_unit(npos);
                m_useds.set_bit(npos, false);
                m_units[npos].base = npos;
                m_units[npos].check = npos;
            }
        }

        for (auto npos = beg_npos; npos < end_npos; npos += m_l1_size) {
            m_heads[npos >> m_l1_bits] = taboo_npos;
        }
    }

    void expand() {
        const auto old_size = static_cast<std::uint64_t>(m_units.size());
        const auto new_size = old_size + 256;

        for (auto npos = old_size; npos < new_size; ++npos) {
            m_units.push_back({npos + 1, npos - 1});
            m_leaves.push_back(false);
            m_terms.push_back(false);
            m_useds.push_back(false);
        }

        {
            const auto last_npos = m_units[taboo_npos].check;
            m_units[old_size].check = last_npos;
            m_units[last_npos].base = old_size;
            m_units[new_size - 1].base = taboo_npos;
            m_units[taboo_npos].check = new_size - 1;
        }

        for (auto npos = old_size; npos < new_size; npos += m_l1_size) {
            m_heads.push_back(npos);
        }

        const auto bpos = old_size / 256;
        if (free_blocks <= bpos) {
            close_block(bpos - free_blocks);
        }
    }

    void arrange(std::uint64_t beg, std::uint64_t end, std::uint64_t kpos, std::uint64_t npos) {
        if (m_keys[beg].size() == kpos) {
            m_terms.set_bit(npos, true);
            if (++beg == end) {  // without link?
                m_units[npos].base = 0;  // with an empty suffix
                m_leaves.set_bit(npos, true);
                return;
            }
        } else if (beg + 1 == end) {  // leaf?
            XCDAT_THROW_IF(m_keys[beg].size() <= kpos, "The input keys are not unique.");
            m_terms.set_bit(npos, true);
            m_leaves.set_bit(npos, true);
            m_suffixes.set_suffix({m_keys[beg].data() + kpos, m_keys[beg].size() - kpos}, npos);
            return;
        }

        // fetching edges
        {
            m_edges.clear();
            auto ch = static_cast<std::uint8_t>(m_keys[beg][kpos]);
            for (auto i = beg + 1; i < end; ++i) {
                const auto next_ch = static_cast<std::uint8_t>(m_keys[i][kpos]);
                if (ch != next_ch) {
                    XCDAT_THROW_IF(next_ch < ch, "The input keys are not in lexicographical order.");
                    m_edges.push_back(ch);
                    ch = next_ch;
                }
            }
            m_edges.push_back(ch);
        }

        const auto base = xcheck(npos >> m_l1_bits);
        if (m_units.size() <= base) {
            expand();
        }

        // defining new edges
        m_units[npos].base = base;
        for (const auto ch : m_edges) {
            const auto child_id = base ^ m_table.get_code(ch);
            use_unit(child_id);
            m_units[child_id].check = npos;
        }

        // following the children
        auto i = beg;
        auto ch = static_cast<uint8_t>(m_keys[beg][kpos]);
        for (auto j = beg + 1; j < end; ++j) {
            const auto next_ch = static_cast<uint8_t>(m_keys[j][kpos]);
            if (ch != next_ch) {
                arrange(i, j, kpos + 1, base ^ m_table.get_code(ch));
                ch = next_ch;
                i = j;
            }
        }
        arrange(i, end, kpos + 1, base ^ m_table.get_code(ch));
    }

    inline std::uint64_t xcheck(std::uint64_t lpos) const {
        if (m_units[taboo_npos].base == taboo_npos) {  // Full?
            return m_units.size() ^ m_table.get_code(m_edges[0]);
        }

        // First, search in the same L1 block
        for (auto i = m_heads[lpos]; i != taboo_npos && i >> m_l1_bits == lpos; i = m_units[i].base) {
            const auto base = i ^ m_table.get_code(m_edges[0]);
            if (is_target(base)) {
                return base;  // base / block_size_ == lpos
            }
        }

        // Second, search in the other blocks
        for (auto i = m_units[taboo_npos].base; i != taboo_npos; i = m_units[i].base) {
            const auto base = i ^ m_table.get_code(m_edges[0]);
            if (is_target(base)) {
                return base;  // base / block_size_ != lpos
            }
        }
        return m_units.size() ^ m_table.get_code(m_edges[0]);
    }

    inline bool is_target(std::uint64_t base) const {
        for (const auto ch : m_edges) {
            if (m_useds[base ^ m_table.get_code(ch)]) {
                return false;
            }
        }
        return true;
    }
};

}  // namespace xcdat