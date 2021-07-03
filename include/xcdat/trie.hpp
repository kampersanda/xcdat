#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "trie_builder.hpp"

namespace xcdat {

//! A compressed string dictionary based on an improved double-array trie.
//! 'BcVector' is the data type of Base and Check vectors.
template <class BcVector>
class trie {
  public:
    using trie_type = trie<BcVector>;
    using bc_vector_type = BcVector;

    static constexpr auto l1_bits = bc_vector_type::l1_bits;

  private:
    std::uint64_t m_num_keys = 0;
    code_table m_table;
    bit_vector m_terms;
    bc_vector_type m_bcvec;
    tail_vector m_tvec;

  public:
    //! Default constructor
    trie() = default;

    //! Default destructor
    virtual ~trie() = default;

    //! Copy constructor (deleted)
    trie(const trie&) = delete;

    //! Copy constructor (deleted)
    trie& operator=(const trie&) = delete;

    //! Move constructor
    trie(trie&&) noexcept = default;

    //! Move constructor
    trie& operator=(trie&&) noexcept = default;

    //! Build the trie from the input keywords, which are lexicographically sorted and unique.
    //!
    //! If bin_mode = false, the NULL character is used for the termination of a keyword.
    //! If bin_mode = true, bit flags are used istead, and the keywords can contain NULL characters.
    //! If the input keywords contain NULL characters, bin_mode will be forced to be set to true.
    //!
    //! The type 'Strings' and 'Strings::value_type' should be a random iterable container such as std::vector.
    //! Precisely, they should support the following operations:
    //!  - size() returns the container size.
    //!  - operator[](i) accesses the i-th element.
    //!  - begin() returns the iterator to the beginning.
    //!  - end() returns the iterator to the end.
    //! The type 'Strings::value_type::value_type' should be one-byte integer type such as 'char'.
    template <class Strings>
    trie(const Strings& keys, bool bin_mode = false) : trie(trie_builder(keys, l1_bits, bin_mode)) {
        static_assert(sizeof(char) == sizeof(typename Strings::value_type::value_type));
    }

    //! Check if the binary mode.
    inline bool bin_mode() const {
        return m_tvec.bin_mode();
    }

    //! Get the number of stored keywords.
    inline std::uint64_t num_keys() const {
        return m_num_keys;
    }

    //! Get the alphabet size.
    inline std::uint64_t alphabet_size() const {
        return m_table.alphabet_size();
    }

    //! Get the maximum length of keywords.
    inline std::uint64_t max_length() const {
        return m_table.max_length();
    }

    //! Get the number of trie nodes.
    inline std::uint64_t num_nodes() const {
        return m_bcvec.num_nodes();
    }

    //! Get the number of DA units.
    inline std::uint64_t num_units() const {
        return m_bcvec.num_units();
    }

    //! Get the number of unused DA units.
    inline std::uint64_t num_free_units() const {
        return m_bcvec.num_free_units();
    }

    //! Get the length of TAIL vector.
    inline std::uint64_t tail_length() const {
        return m_tvec.size();
    }

    //! Lookup the ID of the keyword.
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
        if (!m_tvec.match(get_suffix(key, kpos), tpos)) {
            return std::nullopt;
        }
        return npos_to_id(npos);
    }

    //! Decode the keyword associated with the ID.
    inline std::string decode(std::uint64_t id) const {
        std::string decoded;
        decoded.reserve(max_length());
        decode(id, decoded);
        return decoded;
    }

    //! Decode the keyword associated with the ID and store it in 'decoded'.
    //! It can avoid reallocation of memory to store the result.
    inline void decode(std::uint64_t id, std::string& decoded) const {
        decoded.clear();

        if (num_keys() <= id) {
            return;
        }

        std::uint64_t npos = id_to_npos(id);
        std::uint64_t tpos = m_bcvec.is_leaf(npos) ? m_bcvec.link(npos) : UINT64_MAX;

        while (npos != 0) {
            const std::uint64_t ppos = m_bcvec.check(npos);
            decoded.push_back(m_table.get_char(m_bcvec.base(ppos) ^ npos));
            npos = ppos;
        }

        std::reverse(decoded.begin(), decoded.end());
        if (tpos != 0 && tpos != UINT64_MAX) {
            m_tvec.decode(tpos, [&](char c) { decoded.push_back(c); });
        }
    }

    //! An iterator class for common prefix search.
    //! It enumerates all the keywords contained as prefixes of a given string.
    //! It should be instantiated via the function 'make_prefix_iterator'.
    class prefix_iterator {
      private:
        const trie_type* m_obj = nullptr;
        std::string_view m_key;
        std::uint64_t m_id = 0;
        std::uint64_t m_kpos = 0;
        std::uint64_t m_npos = 0;
        bool is_beg = true;
        bool is_end = false;

      public:
        prefix_iterator() = default;

        //! Increment the iterator.
        //! Return false if the iteration is terminated.
        inline bool next() {
            return m_obj != nullptr && m_obj->next_prefix(this);
        }

        //! Get the result ID.
        inline std::uint64_t id() const {
            return m_id;
        }

        //! Get the result keyword.
        inline std::string decoded() const {
            return std::string(m_key.data(), m_kpos);
        }

        //! Get the reference to the result keyword.
        //! Note that the referenced data will be changed in the next iteration.
        inline std::string_view decoded_view() const {
            return std::string_view(m_key.data(), m_kpos);
        }

      private:
        prefix_iterator(const trie_type* obj, std::string_view key) : m_obj(obj), m_key(key) {}

        friend class trie;
    };

    //! Make the common prefix searcher for the given keyword.
    inline prefix_iterator make_prefix_iterator(std::string_view key) const {
        return prefix_iterator(this, key);
    }

    //! Preform common prefix search for the keyword.
    inline void prefix_search(std::string_view key,
                              const std::function<void(std::uint64_t, std::string_view)>& fn) const {
        auto itr = make_prefix_iterator(key);
        while (itr.next()) {
            fn(itr.id(), itr.decoded_view());
        }
    }

    //! An iterator class for predictive search.
    //! It enumerates all the keywords starting with a given string.
    //! It should be instantiated via the function 'make_predictive_iterator'.
    class predictive_iterator {
      public:
        struct cursor_type {
            char label;
            std::uint64_t kpos;
            std::uint64_t npos;
        };

      private:
        const trie_type* m_obj = nullptr;
        std::string_view m_key;
        std::uint64_t m_id = 0;
        std::string m_decoded;
        std::vector<cursor_type> m_stack;
        bool is_beg = true;
        bool is_end = false;

      public:
        predictive_iterator() = default;

        //! Increment the iterator.
        //! Return false if the iteration is terminated.
        inline bool next() {
            return m_obj != nullptr && m_obj->next_predictive(this);
        }

        //! Get the result ID.
        inline std::uint64_t id() const {
            return m_id;
        }

        //! Get the result keyword.
        inline std::string decoded() const {
            return m_decoded;
        }

        //! Get the reference to the result keyword.
        //! Note that the referenced data will be changed in the next iteration.
        inline std::string_view decoded_view() const {
            return m_decoded;
        }

      private:
        predictive_iterator(const trie_type* obj, std::string_view key) : m_obj(obj), m_key(key) {}

        friend class trie;
    };

    //! Make the predictive searcher for the keyword.
    inline predictive_iterator make_predictive_iterator(std::string_view key) const {
        return predictive_iterator(this, key);
    }

    //! Preform predictive search for the keyword.
    inline void predictive_search(std::string_view key,
                                  const std::function<void(std::uint64_t, std::string_view)>& fn) const {
        auto itr = make_predictive_iterator(key);
        while (itr.next()) {
            fn(itr.id(), itr.decoded_view());
        }
    }

    //! An iterator class for enumeration.
    //! It enumerates all the keywords stored in the trie.
    //! It should be instantiated via the function 'make_enumerative_iterator'.
    using enumerative_iterator = predictive_iterator;

    //! Make the enumerator.
    inline enumerative_iterator make_enumerative_iterator() const {
        return enumerative_iterator(this, "");
    }

    //! Enumerate all the keywords and their IDs stored in the trie.
    inline void enumerate(const std::function<void(std::uint64_t, std::string_view)>& fn) const {
        auto itr = make_enumerative_iterator();
        while (itr.next()) {
            fn(itr.id(), itr.decoded_view());
        }
    }

    //! Visit the members (commonly used for I/O).
    template <class Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(m_num_keys);
        visitor.visit(m_table);
        visitor.visit(m_terms);
        visitor.visit(m_bcvec);
        visitor.visit(m_tvec);
    }

  private:
    template <class Strings>
    explicit trie(trie_builder<Strings>&& b)
        : m_num_keys(b.m_keys.size()), m_table(std::move(b.m_table)), m_terms(b.m_terms, true, true),
          m_bcvec(b.m_units, std::move(b.m_leaves)), m_tvec(std::move(b.m_suffixes)) {}

    template <class String>
    static constexpr String get_suffix(const String& s, std::uint64_t i) {
        assert(i <= s.size());
        return s.substr(i, s.size() - i);
    }

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

        if (bin_mode() and itr->m_kpos == itr->m_key.size()) {
            // Is the key terminated at an inner term?
            itr->is_end = true;
            itr->m_id = num_keys();
            return false;
        }

        while (!m_bcvec.is_leaf(itr->m_npos)) {
            if (bin_mode() and itr->m_kpos == itr->m_key.size()) {
                // Is the key terminated at an internal node (not term)?
                itr->is_end = true;
                itr->m_id = num_keys();
                return false;
            }

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
        if (!m_tvec.match(get_suffix(itr->m_key, itr->m_kpos), tpos)) {
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
                    const std::uint64_t tpos = m_bcvec.link(npos);
                    if (tpos == 0) {
                        return false;
                    }
                    if (!m_tvec.prefix_match(get_suffix(itr->m_key, kpos), tpos)) {
                        return false;
                    }
                    itr->m_id = npos_to_id(npos);
                    m_tvec.decode(tpos, [&](char c) { itr->m_decoded.push_back(c); });
                    return true;
                }

                const std::uint64_t cpos = m_bcvec.base(npos) ^ m_table.get_code(itr->m_key[kpos]);
                if (m_bcvec.check(cpos) != npos) {
                    itr->is_end = true;
                    return false;
                }

                npos = cpos;
                itr->m_decoded.push_back(itr->m_key[kpos]);
            }

            if (!itr->m_decoded.empty()) {
                itr->m_stack.push_back({itr->m_decoded.back(), kpos, npos});
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
                itr->m_decoded.resize(kpos);
                itr->m_decoded.back() = label;
            }

            if (m_bcvec.is_leaf(npos)) {
                itr->m_id = npos_to_id(npos);
                m_tvec.decode(m_bcvec.link(npos), [&](char c) { itr->m_decoded.push_back(c); });
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
