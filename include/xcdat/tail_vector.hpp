#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "bit_vector.hpp"
#include "exception.hpp"
#include "mm_vector.hpp"

namespace xcdat {

//! TAIL implementation with suffix merge
class tail_vector {
  public:
    struct suffix_type {
        std::string_view str;
        std::uint64_t npos;

        inline char operator[](std::uint64_t i) const {
            return str[size() - i - 1];
        }
        inline std::uint64_t size() const {
            return str.size();
        }

        inline const char* begin() const {
            return str.data();
        }
        inline const char* end() const {
            return str.data() + str.size();
        }

        inline std::reverse_iterator<const char*> rbegin() const {
            return std::make_reverse_iterator(str.data() + str.size());
        }
        inline std::reverse_iterator<const char*> rend() const {
            return std::make_reverse_iterator(str.data());
        }
    };

    class builder {
      private:
        // Buffer
        std::vector<suffix_type> m_suffixes;

        // Released
        std::vector<char> m_chars;
        bit_vector::builder m_term_flags;

      public:
        builder() = default;

        virtual ~builder() = default;

        //!
        void set_suffix(std::string_view str, std::uint64_t npos) {
            XCDAT_THROW_IF(str.size() == 0, "The given suffix is empty.");
            m_suffixes.push_back({str, npos});
        }

        // setter(npos, tpos): Set units[npos].base = tpos.
        void complete(bool bin_mode, const std::function<void(std::uint64_t, std::uint64_t)>& setter) {
            std::sort(m_suffixes.begin(), m_suffixes.end(), [](const suffix_type& a, const suffix_type& b) {
                return std::lexicographical_compare(std::rbegin(a), std::rend(a), std::rbegin(b), std::rend(b));
            });

            // Dummy for an empty suffix
            m_chars.emplace_back('\0');
            if (bin_mode) {
                m_term_flags.push_back(false);
            }

            const suffix_type dmmy_suffix = {{nullptr, 0}, 0};
            const suffix_type* prev_suffix = &dmmy_suffix;

            std::uint64_t prev_tpos = 0;

            for (std::uint64_t i = m_suffixes.size(); i > 0; --i) {
                const suffix_type& curr_suffix = m_suffixes[i - 1];
                XCDAT_THROW_IF(curr_suffix.size() == 0, "A suffix is empty.");

                std::uint64_t match = 0;
                while ((match < curr_suffix.size()) && (match < prev_suffix->size()) &&
                       ((*prev_suffix)[match] == curr_suffix[match])) {
                    ++match;
                }

                if ((match == curr_suffix.size()) && (prev_suffix->size() != 0)) {  // sharable
                    setter(curr_suffix.npos, prev_tpos + (prev_suffix->size() - match));
                    prev_tpos += prev_suffix->size() - match;
                } else {  // append
                    setter(curr_suffix.npos, m_chars.size());
                    prev_tpos = m_chars.size();
                    std::copy(curr_suffix.begin(), curr_suffix.end(), std::back_inserter(m_chars));
                    if (bin_mode) {
                        for (std::uint64_t j = 1; j < curr_suffix.size(); ++j) {
                            m_term_flags.push_back(false);
                        }
                        m_term_flags.push_back(true);
                    } else {
                        m_chars.emplace_back('\0');
                    }
                }

                prev_suffix = &curr_suffix;
            }
        }

        friend class tail_vector;
    };

  private:
    mm_vector<char> m_chars;
    bit_vector m_term_flags;

  public:
    tail_vector() = default;

    explicit tail_vector(builder& b) {
        m_chars.steal(b.m_chars);
        m_term_flags.build(b.m_term_flags);
    }

    ~tail_vector() = default;

    inline bool bin_mode() const {
        return m_term_flags.size() != 0;
    }

    inline bool match(std::string_view key, std::uint64_t tpos) const {
        if (key.size() == 0) {
            return tpos == 0;
        }

        std::uint64_t kpos = 0;

        if (bin_mode()) {
            do {
                if (key[kpos] != m_chars[tpos]) {
                    return false;
                }
                kpos += 1;
                if (m_term_flags[tpos]) {
                    return kpos == key.size();
                }
                tpos += 1;
            } while (kpos < key.size());
            return false;
        } else {
            do {
                if (!m_chars[tpos] || key[kpos] != m_chars[tpos]) {
                    return false;
                }
                kpos += 1;
                tpos += 1;
            } while (kpos < key.size());
            return !m_chars[tpos];
        }
    }

    inline bool prefix_match(std::string_view key, std::uint64_t& tpos) const {
        if (key.size() == 0) {
            return true;
        }

        std::uint64_t kpos = 0;

        if (bin_mode()) {
            do {
                if (key[kpos] != m_chars[tpos]) {
                    return false;
                }
                kpos += 1;
                if (m_term_flags[tpos]) {
                    return kpos == key.size();
                }
                tpos += 1;
            } while (kpos < key.size());
            return true;
        } else {
            do {
                if (!m_chars[tpos] || key[kpos] != m_chars[tpos]) {
                    return false;
                }
                kpos += 1;
                tpos += 1;
            } while (kpos < key.size());
            return true;
        }
    }

    inline void decode(std::uint64_t tpos, const std::function<void(char)>& fn) const {
        if (bin_mode()) {
            do {
                fn(m_chars[tpos]);
            } while (!m_term_flags[tpos++]);
        } else {
            do {
                fn(m_chars[tpos++]);
            } while (m_chars[tpos]);
        }
    }

    inline std::uint64_t size() const {
        return m_chars.size();
    }
};

}  // namespace xcdat