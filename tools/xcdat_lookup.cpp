#include <xcdat.hpp>

#include "cmd_line_parser/parser.hpp"
#include "mm_file/mm_file.hpp"
#include "tinyformat/tinyformat.h"

cmd_line_parser::parser make_parser(int argc, char** argv) {
    cmd_line_parser::parser p(argc, argv);
    p.add("input_idx", "Input filepath of trie index");
    return p;
}

xcdat::flag_type get_flag(std::string_view filepath) {
    std::ifstream ifs(filepath);
    XCDAT_THROW_IF(!ifs.good(), "Cannot open the input file");
    xcdat::flag_type flag;
    ifs.read(reinterpret_cast<char*>(&flag), sizeof(flag));
    return flag;
}

template <class Trie>
int lookup(const cmd_line_parser::parser& p) {
    const auto input_idx = p.get<std::string>("input_idx");

    const mm::file_source<char> fin(input_idx.c_str(), mm::advice::sequential);
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

    const auto input_idx = p.get<std::string>("input_idx");
    const auto flag = get_flag(input_idx);

    switch (flag) {
        case 7:
            return lookup<xcdat::trie_7_type>(p);
        case 8:
            return lookup<xcdat::trie_8_type>(p);
        default:
            break;
    }

    p.help();
    return 1;
}