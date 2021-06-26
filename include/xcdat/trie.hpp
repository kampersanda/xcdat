#pragma once

#include <functional>
#include <optional>
#include <string>

#include "bc_vector.hpp"
#include "trie_builder.hpp"

namespace xcdat {

class trie {
  private:
    std::uint64_t m_num_keys = 0;
    code_table m_table;
    bit_vector m_terms;
    bc_vector m_bcvec;
    tail_vector m_tvec;

  public:
    trie() = default;

    virtual ~trie() = default;

    static trie build(const std::vector<std::string>& keys, bool bin_mode = false) {
        trie_builder b(keys, bc_vector::l1_bits, bin_mode);
        return trie(b);
    }

    inline std::uint64_t num_keys() const {
        return m_num_keys;
    }

    inline bool bin_mode() const {
        return m_tvec.bin_mode();
    }

    inline std::uint64_t alphabet_size() const {
        return m_table.alphabet_size();
    }

    inline std::uint64_t max_length() const {
        return m_table.max_length();
    }

    inline std::optional<std::uint64_t> lookup(std::string_view key) const {
        std::uint64_t kpos = 0, npos = 0;
        while (!m_bcvec.is_leaf(npos)) {
            if (kpos == key.size()) {
                if (!m_terms[npos]) {
                    return std::nullopt;
                }
                return npos_to_id(npos);
            }
            const auto cpos = m_bcvec.base(npos) ^ m_table.get_code(key[kpos++]);
            if (m_bcvec.check(cpos) != npos) {
                return std::nullopt;
            }
            npos = cpos;
        }

        const std::uint64_t tpos = m_bcvec.link(npos);
        if (!m_tvec.match(key.substr(kpos, key.size() - kpos), tpos)) {
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
        auto tpos = m_bcvec.is_leaf(npos) ? m_bcvec.link(npos) : UINT64_MAX;

        while (npos != 0) {
            const auto ppos = m_bcvec.check(npos);
            decoded.push_back(m_table.get_char(m_bcvec.base(ppos) ^ npos));
            npos = ppos;
        }

        std::reverse(decoded.begin(), decoded.end());

        if (tpos != 0 && tpos != UINT64_MAX) {
            m_tvec.decode(tpos, [&](char c) { decoded.push_back(c); });
        }
        return decoded;
    }

  private:
    trie(trie_builder& b)
        : m_num_keys(b.m_keys.size()), m_table(b.m_table), m_terms(b.m_terms, true, true),
          m_bcvec(b.m_units, std::move(b.m_leaves)), m_tvec(b.m_suffixes) {}

    inline std::uint64_t npos_to_id(std::uint64_t npos) const {
        return m_terms.rank(npos);
    };

    inline std::uint64_t id_to_npos(std::uint64_t id) const {
        return m_terms.select(id);
    };
};

}  // namespace xcdat