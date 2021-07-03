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
int lookup(const cmd_line_parser::parser& p) {
    const auto input_dic = p.get<std::string>("input_dic");

    const mm::file_source<char> fin(input_dic.c_str(), mm::advice::sequential);
    const auto trie = xcdat::mmap<Trie>(fin.data());

    for (std::string str; std::getline(std::cin, str);) {
        const auto id = trie.lookup(str);
        if (id.has_value()) {
            tfm::printfln("%d\t%s", id.value(), str);
        } else {
            tfm::printfln("-1\t%s", str);
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
            return lookup<xcdat::trie_7_type>(p);
        case 8:
            return lookup<xcdat::trie_8_type>(p);
        case 15:
            return lookup<xcdat::trie_15_type>(p);
        case 16:
            return lookup<xcdat::trie_16_type>(p);
        default:
            break;
    }

    p.help();
    return 1;
}