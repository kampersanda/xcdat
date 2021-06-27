#include <xcdat.hpp>

#include "cmd_line_parser/parser.hpp"
#include "tinyformat/tinyformat.h"

cmd_line_parser::parser make_parser(int argc, char** argv) {
    cmd_line_parser::parser p(argc, argv);
    p.add("input_idx", "Input filepath of trie index");
    p.add("trie_type", "Type of trie impl. from [7|8], (default=7)", "-t", false);
    return p;
}

template <class Trie>
int decode(const cmd_line_parser::parser& p) {
    const auto input_idx = p.get<std::string>("input_idx");
    const auto trie = Trie::load(input_idx);

    for (std::uint64_t id; std::cin >> id;) {
        const auto dec = trie.decode(id);
        tfm::printfln("%d\t%s", id, dec);
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

    const auto trie_type = p.get<int>("trie_type", 7);

    switch (trie_type) {
        case 7:
            return decode<xcdat::trie_7_type>(p);
        case 8:
            return decode<xcdat::trie_8_type>(p);
        default:
            break;
    }

    p.help();
    return 1;
}