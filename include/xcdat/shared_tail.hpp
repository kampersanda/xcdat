#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "bit_vector.hpp"
#include "mm_vector.hpp"

namespace xcdat {

//! TAIL implementation with suffix merge
class shared_tail {
  public:
    struct suffix_type {
        std::string_view str;
        std::uint64_t npos;

        inline char operator[](std::uint64_t i) const {
            return str[length() - i - 1];
        }
        inline std::uint64_t length() const {
            return str.length();
        }

        inline const char* begin() const {
            return str.data();
        }
        inline const char* end() const {
            return str.data() + str.length();
        }

        inline std::reverse_iterator<const char*> rbegin() const {
            return std::make_reverse_iterator(str.data() + str.length());
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

        void set_suffix(std::string_view str, std::uint64_t npos) {
            m_suffixes.push_back({str, npos});
        }

        // setter(npos, tpos): Set units[npos].base = tpos.
        void complete(bool bin_mode, const std::function<void(std::uint64_t, std::uint64_t)>& setter) {
            std::sort(m_suffixes.begin(), m_suffixes.end(), [](const suffix_type& a, const suffix_type& b) {
                return std::lexicographical_compare(std::rbegin(a), std::rend(a), std::rbegin(b), std::rend(b));
            });

            // Sentinel for an empty suffix
            m_chars.emplace_back('\0');
            if (bin_mode) {
                m_term_flags.push_back(false);
            }

            const suffix_type dmmy_suffix = {{nullptr, 0}, 0};
            const suffix_type* prev_suffix = &dmmy_suffix;

            std::uint64_t prev_tpos = 0;

            for (std::uint64_t i = m_suffixes.size(); i > 0; --i) {
                const suffix_type& curr_suffix = m_suffixes[i - 1];
                if (curr_suffix.length() == 0) {
                    // throw TrieBuilder::Exception("A suffix is empty.");
                }

                std::uint64_t match = 0;
                while ((match < curr_suffix.length()) && (match < prev_suffix->length()) &&
                       ((*prev_suffix)[match] == curr_suffix[match])) {
                    ++match;
                }

                if ((match == curr_suffix.length()) && (prev_suffix->length() != 0)) {  // sharable
                    setter(curr_suffix.npos, prev_tpos + (prev_suffix->length() - match));
                    prev_tpos += prev_suffix->length() - match;
                } else {  // append
                    setter(curr_suffix.npos, m_chars.size());
                    prev_tpos = m_chars.size();
                    std::copy(curr_suffix.begin(), curr_suffix.end(), std::back_inserter(m_chars));
                    if (bin_mode) {
                        for (std::uint64_t j = 1; j < curr_suffix.length(); ++j) {
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

        friend class shared_tail;
    };

  private:
    mm_vector<char> m_chars;
    bit_vector m_term_flags;

  public:
    shared_tail() = default;

    explicit shared_tail(builder& b) {
        m_chars.steal(b.m_chars);
        m_term_flags.build(b.m_term_flags);
    }

    ~shared_tail() = default;

    inline bool bin_mode() const {
        return m_term_flags.size() == 0;
    }

    inline bool match(std::string_view key, size_t tpos) const {}

    inline void decode(std::string& decoded, size_t tpos) const {
        if (bin_mode()) {
            do {
                decoded.push_back(m_chars[tpos]);
            } while (!m_term_flags[tpos++]);
        } else {
            do {
                decoded.push_back(m_chars[tpos++]);
            } while (m_chars[tpos]);
        }
    }
};

}  // namespace xcdat