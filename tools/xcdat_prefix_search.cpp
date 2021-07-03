#include <xcdat.hpp>

#include "cmd_line_parser/parser.hpp"
#include "mm_file/mm_file.hpp"
#include "tinyformat/tinyformat.h"

cmd_line_parser::parser make_parser(int argc, char** argv) {
    cmd_line_parser::parser p(argc, argv);
    p.add("input_dic", "Input filepath of trie dictionary");
    return p;
}

template <class Trie>
int prefix_search(const cmd_line_parser::parser& p) {
    const auto input_dic = p.get<std::string>("input_dic");

    const mm::file_source<char> fin(input_dic.c_str(), mm::advice::sequential);
    const auto trie = xcdat::mmap<Trie>(fin.data());

    struct result_type {
        std::uint64_t id;
        std::string_view str;
    };

    std::vector<result_type> results;
    results.reserve(trie.max_length());

    for (std::string key; std::getline(std::cin, key);) {
        results.clear();
        trie.prefix_search(key, [&](std::uint64_t id, std::string_view str) { results.push_back({id, str}); });

        tfm::printfln("%d found", results.size());
        for (const auto& r : results) {
            tfm::printfln("%d\t%s", r.id, r.str);
        }
    }

    return 0;
}

int main(int argc, char** argv) {
#ifndef NDEBUG
    tfm::warnfln("The code is running in debug mode.");
#endif
    std::ios::sync_with_stdio(false);

    auto p = make_parser(argc, argv);
    if (!p.parse()) {
        return 1;
    }

    const auto input_dic = p.get<std::string>("input_dic");
    const auto flag = xcdat::get_flag(input_dic);

    switch (flag) {
        case 7:
            return prefix_search<xcdat::trie_7_type>(p);
        case 8:
            return prefix_search<xcdat::trie_8_type>(p);
        case 15:
            return prefix_search<xcdat::trie_15_type>(p);
        case 16:
            return prefix_search<xcdat::trie_16_type>(p);
        default:
            break;
    }

    p.help();
    return 1;
}