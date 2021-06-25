#pragma once

#include <optional>
#include <string>

#include "dac_bc.hpp"
#include "trie_builder.hpp"

namespace xcdat {

class trie {
  public:
  private:
    std::uint64_t m_num_keys = 0;
    code_table m_table;
    dac_bc m_bc;
    bit_vector m_term_flags;
    shared_tail m_tail;

  public:
    trie() = default;

    virtual ~trie() = default;

    static trie build(const std::vector<std::string_view>& keys, bool bin_mode = false) {
        trie_builder b(keys, 8, bin_mode);
        return trie(b);
    }

    inline std::uint64_t num_keys() const {
        return m_num_keys;
    }

    inline bool bin_mode() const {
        return m_tail.bin_mode();
    }

    inline std::uint64_t alphabet_size() const {
        return m_table.alphabet_size();
    }

    inline std::uint64_t max_length() const {
        return m_table.max_length();
    }

    inline std::optional<std::uint64_t> lookup(std::string_view key) const {
        std::uint64_t kpos = 0, npos = 0;
        while (!m_bc.is_leaf(npos)) {
            if (kpos == key.length()) {
                if (m_term_flags[npos]) {
                    return npos_to_id(npos);
                }
                return std::nullopt;
            }
            const auto cpos = m_bc.base(npos) ^ m_table.get_code(key[kpos++]);
            if (m_bc.check(cpos) != npos) {
                return std::nullopt;
            }
            npos = cpos;
        }
        const std::uint64_t tpos = m_bc.link(npos);
        if (!m_tail.match(key.substr(kpos, key.length() - kpos), tpos)) {
            return std::nullopt;
        }
        return npos_to_id(npos);
    }

    inline std::string access(std::uint64_t id) const {
        if (num_keys() <= id) {
            return {};
        }

        std::string decoded;
        decoded.reserve(max_length());

        auto npos = id_to_npos(id);
        auto tpos = m_bc.is_leaf(npos) ? m_bc.link(npos) : UINT64_MAX;

        while (npos != 0) {
            const auto ppos = m_bc.check(npos);
            decoded.push_back(m_table.get_char(m_bc.base(ppos) ^ npos));
            npos = ppos;
        }

        std::reverse(decoded.begin(), decoded.end());

        if (tpos != 0 && tpos != UINT64_MAX) {
            m_tail.decode(decoded, tpos);
        }
        return decoded;
    }

  private:
    trie(trie_builder& b)
        : m_num_keys(b.m_keys.size()), m_table(b.m_table), m_bc(b.m_units, b.m_leaf_flags),
          m_term_flags(b.m_term_flags, true, true), m_tail(b.m_suffixes) {}

    inline std::uint64_t npos_to_id(std::uint64_t npos) const {
        return m_term_flags.rank(npos);
    };

    inline std::uint64_t id_to_npos(std::uint64_t id) const {
        return m_term_flags.select(id);
    };
};

}  // namespace xcdat