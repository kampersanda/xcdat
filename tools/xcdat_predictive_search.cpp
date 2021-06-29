#include <xcdat.hpp>

#include "cmd_line_parser/parser.hpp"
#include "mm_file/mm_file.hpp"
#include "tinyformat/tinyformat.h"

cmd_line_parser::parser make_parser(int argc, char** argv) {
    cmd_line_parser::parser p(argc, argv);
    p.add("input_idx", "Input filepath of trie index");
    p.add("max_num_results", "The max number of results (default=10)", "-n", false);
    return p;
}

template <class Trie>
int predictive_search(const cmd_line_parser::parser& p) {
    const auto input_idx = p.get<std::string>("input_idx");
    const auto max_num_results = p.get<std::uint64_t>("max_num_results", 10);

    const mm::file_source<char> fin(input_idx.c_str(), mm::advice::sequential);
    const auto trie = xcdat::mmap<Trie>(fin.data());

    struct result_type {
        std::uint64_t id;
        std::string str;
    };
    std::vector<result_type> results;
    results.reserve(1ULL << 10);

    for (std::string str; std::getline(std::cin, str);) {
        results.clear();

        auto itr = trie.make_predictive_iterator(str);
        while (itr.next()) {
            results.push_back({itr.id(), itr.decoded()});
        }

        tfm::printfln("%d found", results.size());
        for (std::uint64_t i = 0; i < std::min<std::uint64_t>(results.size(), max_num_results); i++) {
            const auto& r = results[i];
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

    const auto input_idx = p.get<std::string>("input_idx");
    const auto flag = xcdat::get_flag(input_idx);

    switch (flag) {
        case 7:
            return predictive_search<xcdat::trie_7_type>(p);
        case 8:
            return predictive_search<xcdat::trie_8_type>(p);
        default:
            break;
    }

    p.help();
    return 1;
}