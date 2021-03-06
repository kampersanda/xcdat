#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace cmd_line_parser {

struct parser {
    inline static const std::string empty = "";

    parser(int argc, char** argv) : m_argc(argc), m_argv(argv), m_required(0) {}

    struct cmd {
        std::string shorthand, value, descr;
        bool is_boolean;
    };

    bool parse() {
        if (size_t(m_argc - 1) < m_required) return abort();
        size_t k = 0;
        for (int i = 1; i != m_argc; ++i, ++k) {
            std::string parsed(m_argv[i]);
            if (parsed == "-h" or parsed == "--help") return abort();
            size_t id = k;
            bool is_optional = id >= m_required;
            if (is_optional) {
                auto it = m_shorthands.find(parsed);
                if (it == m_shorthands.end()) {
                    std::cerr << "== error: shorthand '" + parsed + "' not found" << std::endl;
                    return abort();
                }
                id = (*it).second;
            }
            assert(id < m_names.size());
            auto const& name = m_names[id];
            auto& c = m_cmds[name];
            if (is_optional) {
                if (c.is_boolean) {
                    parsed = "true";
                } else {
                    ++i;
                    if (i == m_argc) return abort();
                    parsed = m_argv[i];
                }
            }
            c.value = parsed;
        }
        return true;
    }

    void help() const {
        std::cerr << "Usage: \e[1m" << m_argv[0] << "\e[0m [-h,--help]";
        auto print = [this](bool with_description) {
            for (size_t i = 0; i != m_names.size(); ++i) {
                auto const& c = m_cmds.at(m_names[i]);
                bool is_optional = i >= m_required;
                if (is_optional) std::cerr << " [\e[1m" << c.shorthand << "\e[0m";
                if (!c.is_boolean) std::cerr << " \e[4m" << m_names[i] << "\e[0m";
                if (is_optional) std::cerr << "]";
                if (with_description) std::cerr << "\n\t" << c.descr << "\n";
            }
        };
        print(false);
        std::cerr << "\n\n";
        print(true);
        std::cerr << " [-h,--help]\n\tPrint this help text and silently exits." << std::endl;
    }

    bool add(std::string const& name, std::string const& descr) {
        bool ret = m_cmds.emplace(name, cmd{empty, empty, descr, false}).second;
        if (ret) {
            m_names.push_back(name);
            m_required += 1;
        }
        return ret;
    }

    bool add(std::string const& name, std::string const& descr, std::string const& shorthand, bool is_boolean = true) {
        bool ret = m_cmds.emplace(name, cmd{shorthand, is_boolean ? "false" : empty, descr, is_boolean}).second;
        if (ret) {
            m_names.push_back(name);
            m_shorthands.emplace(shorthand, m_names.size() - 1);
        }
        return ret;
    }

    template <typename T>
    T get(std::string const& name) const {
        auto it = m_cmds.find(name);
        if (it == m_cmds.end()) {
            throw std::runtime_error("error: '" + name + "' not found");
        }
        auto const& value = (*it).second.value;
        return parse<T>(value);
    }

    // added by Kampersanda
    template <typename T>
    T get(std::string const& name, const T& default_value) const {
        return parsed(name) ? get<T>(name) : default_value;
    }

    bool parsed(std::string const& name) const {
        auto it = m_cmds.find(name);
        if (it == m_cmds.end() or (*it).second.value == empty) return false;
        return true;
    }

    template <typename T>
    T parse(std::string const& value) const {
        if constexpr (std::is_same<T, std::string>::value) {
            return value;
        } else if constexpr (std::is_same<T, char>::value or std::is_same<T, signed char>::value or
                             std::is_same<T, unsigned char>::value) {
            return value.front();
        } else if constexpr (std::is_same<T, unsigned int>::value or std::is_same<T, int>::value or
                             std::is_same<T, unsigned short int>::value or std::is_same<T, short int>::value) {
            return std::atoi(value.c_str());
        } else if constexpr (std::is_same<T, unsigned long int>::value or std::is_same<T, long int>::value or
                             std::is_same<T, unsigned long long int>::value or std::is_same<T, long long int>::value) {
            return std::atoll(value.c_str());
        } else if constexpr (std::is_same<T, float>::value or std::is_same<T, double>::value or
                             std::is_same<T, long double>::value) {
            return std::atof(value.c_str());
        } else if constexpr (std::is_same<T, bool>::value) {
            std::istringstream stream(value);
            bool ret;
            if (value == "true" or value == "false") {
                stream >> std::boolalpha >> ret;
            } else {
                stream >> std::noboolalpha >> ret;
            }
            return ret;
        }
        assert(false);
        __builtin_unreachable();
    }

  private:
    int m_argc;
    char** m_argv;
    size_t m_required;
    std::unordered_map<std::string, cmd> m_cmds;
    std::unordered_map<std::string, int> m_shorthands;
    std::vector<std::string> m_names;

    bool abort() const {
        help();
        return false;
    }
};

}  // namespace cmd_line_parser