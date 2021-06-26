#pragma once

#include <functional>
#include <optional>
#include <string>

#include "bc_vector.hpp"
#include "trie_builder.hpp"
#include "utils.hpp"

namespace xcdat {

class trie {
  public:
    using type = trie;

  private:
    std::uint64_t m_num_keys = 0;
    code_table m_table;
    bit_vector m_terms;
    bc_vector m_bcvec;
    tail_vector m_tvec;

  public:
    trie() = default;

    virtual ~trie() = default;

    template <class Strings>
    static trie build(const Strings& keys, bool bin_mode = false) {
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
            const std::uint64_t cpos = m_bcvec.base(npos) ^ m_table.get_code(key[kpos++]);
            if (m_bcvec.check(cpos) != npos) {
                return std::nullopt;
            }
            npos = cpos;
        }

        const std::uint64_t tpos = m_bcvec.link(npos);
        if (!m_tvec.match(utils::get_suffix(key, kpos), tpos)) {
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

    class prefix_iterator {
      private:
        const type* m_obj = nullptr;
        std::string_view m_key;
        std::uint64_t m_id = 0;
        std::uint64_t m_kpos = 0;
        std::uint64_t m_npos = 0;
        bool is_beg = true;
        bool is_end = false;

      public:
        prefix_iterator() = default;

        inline bool next() {
            return m_obj != nullptr && m_obj->next_prefix(this);
        }

        inline std::uint64_t id() const {
            return m_id;
        }
        inline std::string_view prefix() const {
            return {m_key.data(), m_kpos};
        }

      private:
        prefix_iterator(const type* obj, std::string_view key) : m_obj(obj), m_key(key) {}

        friend class trie;
    };

    inline prefix_iterator make_prefix_iterator(std::string_view key) const {
        return prefix_iterator(this, key);
    }

    class predictive_iterator {
      public:
        struct cursor_type {
            char label;
            std::uint64_t kpos;
            std::uint64_t npos;
        };

      private:
        const type* m_obj = nullptr;
        std::string_view m_key;
        std::uint64_t m_id = 0;
        std::string m_prefix;
        std::vector<cursor_type> m_stack;
        bool is_beg = true;
        bool is_end = false;

      public:
        predictive_iterator() = default;

        inline bool next() {
            return m_obj != nullptr && m_obj->next_predictive(this);
        }

        inline std::uint64_t id() const {
            return m_id;
        }
        inline std::string_view prefix() const {
            return m_prefix;
        }

      private:
        predictive_iterator(const type* obj, std::string_view key) : m_obj(obj), m_key(key) {}

        friend class trie;
    };

    inline predictive_iterator make_predictive_iterator(std::string_view key) const {
        return predictive_iterator(this, key);
    }

  private:
    template <class Strings>
    trie(trie_builder<Strings>& b)
        : m_num_keys(b.m_keys.size()), m_table(b.m_table), m_terms(b.m_terms, true, true),
          m_bcvec(b.m_units, std::move(b.m_leaves)), m_tvec(b.m_suffixes) {}

    inline std::uint64_t npos_to_id(std::uint64_t npos) const {
        return m_terms.rank(npos);
    };

    inline std::uint64_t id_to_npos(std::uint64_t id) const {
        return m_terms.select(id);
    };

    inline bool next_prefix(prefix_iterator* itr) const {
        if (itr->is_end) {
            return false;
        }

        if (itr->is_beg) {
            itr->is_beg = false;
            if (m_terms[itr->m_npos]) {
                itr->m_id = npos_to_id(itr->m_npos);
                return true;
            }
        }

        while (!m_bcvec.is_leaf(itr->m_npos)) {
            const std::uint64_t cpos = m_bcvec.base(itr->m_npos) ^ m_table.get_code(itr->m_key[itr->m_kpos++]);
            if (m_bcvec.check(cpos) != itr->m_npos) {
                itr->is_end = true;
                itr->m_id = num_keys();
                return false;
            }
            itr->m_npos = cpos;
            if (!m_bcvec.is_leaf(itr->m_npos) && m_terms[itr->m_npos]) {
                itr->m_id = npos_to_id(itr->m_npos);
                return true;
            }
        }
        itr->is_end = true;

        const std::uint64_t tpos = m_bcvec.link(itr->m_npos);
        if (!m_tvec.match(utils::get_suffix(itr->m_key, itr->m_kpos), tpos)) {
            itr->m_id = num_keys();
            return false;
        }

        itr->m_kpos = itr->m_key.size();
        itr->m_id = npos_to_id(itr->m_npos);
        return true;
    }

    inline bool next_predictive(predictive_iterator* itr) const {
        if (itr->is_end) {
            return false;
        }

        if (itr->is_beg) {
            itr->is_beg = false;

            std::uint64_t kpos = 0;
            std::uint64_t npos = 0;

            for (; kpos < itr->m_key.size(); ++kpos) {
                if (m_bcvec.is_leaf(npos)) {
                    itr->is_end = true;

                    std::uint64_t tpos = m_bcvec.link(npos);
                    if (tpos == 0) {
                        return false;
                    }

                    if (!m_tvec.prefix_match(utils::get_suffix(itr->m_key, kpos), tpos)) {
                        return false;
                    }

                    itr->m_id = npos_to_id(npos);
                    m_tvec.decode(tpos, [&](char c) { itr->m_prefix.push_back(c); });

                    return true;
                }

                const std::uint64_t cpos = m_bcvec.base(npos) ^ m_table.get_code(itr->m_key[kpos]);

                if (m_bcvec.check(cpos) != npos) {
                    itr->is_end = true;
                    return false;
                }

                npos = cpos;
                itr->m_prefix.push_back(itr->m_key[kpos]);
            }

            if (!itr->m_prefix.empty()) {
                itr->m_stack.push_back({itr->m_prefix.back(), kpos, npos});
            } else {
                itr->m_stack.push_back({'\0', kpos, npos});
            }
        }

        while (!itr->m_stack.empty()) {
            const char label = itr->m_stack.back().label;
            const std::uint64_t kpos = itr->m_stack.back().kpos;
            const std::uint64_t npos = itr->m_stack.back().npos;

            itr->m_stack.pop_back();

            if (0 < kpos) {
                itr->m_prefix.resize(kpos);
                itr->m_prefix.back() = label;
            }

            if (m_bcvec.is_leaf(npos)) {
                itr->m_id = npos_to_id(npos);
                m_tvec.decode(m_bcvec.link(npos), [&](char c) { itr->m_prefix.push_back(c); });
                return true;
            }

            const std::uint64_t base = m_bcvec.base(npos);

            for (auto cit = m_table.rbegin(); cit != m_table.rend(); ++cit) {
                const std::uint64_t cpos = base ^ m_table.get_code(*cit);
                if (m_bcvec.check(cpos) == npos) {
                    itr->m_stack.push_back({static_cast<char>(*cit), kpos + 1, cpos});
                }
            }

            if (m_terms[npos]) {
                itr->m_id = npos_to_id(npos);
                return true;
            }
        }

        itr->is_end = true;
        return false;
    }
};

}  // namespace xcdat