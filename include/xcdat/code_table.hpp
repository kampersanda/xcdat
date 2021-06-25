#pragma once

#include <array>
#include <string_view>

#include "mm_vector.hpp"

namespace xcdat {

class code_table {
  private:
    std::uint64_t m_max_length = 0;
    std::array<std::uint8_t, 512> m_table;
    mm_vector<std::uint8_t> m_alphabet;

    struct cf_type {
        std::uint8_t ch;
        std::uint64_t freq;
    };

  public:
    code_table() = default;

    virtual ~code_table() = default;

    void build(const std::vector<std::string_view>& keys) {
        std::array<cf_type, 256> counter;
        for (std::uint32_t ch = 0; ch < 256; ++ch) {
            counter[ch] = {static_cast<std::uint8_t>(ch), 0};
        }

        m_max_length = 0;
        for (const auto& key : keys) {
            for (std::uint8_t ch : key) {
                counter[ch].freq += 1;
            }
            m_max_length = std::max<std::uint64_t>(m_max_length, key.length());
        }

        {
            std::vector<std::uint8_t> alphabet;
            for (const auto& cf : counter) {
                if (cf.freq != 0) {
                    alphabet.push_back(cf.ch);
                }
            }
            m_alphabet.steal(alphabet);
        }

        std::sort(counter.begin(), counter.end(), [](const cf_type& a, const cf_type& b) { return a.freq > b.freq; });

        for (std::uint32_t ch = 0; ch < 256; ++ch) {
            m_table[counter[ch].ch] = static_cast<std::uint8_t>(ch);
        }
        for (std::uint32_t ch = 0; ch < 256; ++ch) {
            m_table[m_table[ch] + 256] = static_cast<std::uint8_t>(ch);
        }
    }

    inline std::uint64_t alphabet_size() const {
        return m_alphabet.size();
    }

    inline std::uint64_t max_length() const {
        return m_max_length;
    }

    inline std::uint8_t get_code(char ch) const {
        return m_table[static_cast<std::uint8_t>(ch)];
    }

    inline char get_char(std::uint8_t cd) const {
        return static_cast<char>(m_table[cd + 256]);
    }

    inline auto begin() const {
        return m_alphabet.begin();
    }

    inline auto end() const {
        return m_alphabet.end();
    }
};

}  // namespace xcdat
