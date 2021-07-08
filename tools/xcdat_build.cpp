#include <xcdat.hpp>

#include "cmd_line_parser/parser.hpp"
#include "tinyformat/tinyformat.h"

cmd_line_parser::parser make_parser(int argc, char** argv) {
    cmd_line_parser::parser p(argc, argv);
    p.add("input_keys", "Input filepath of keywords");
    p.add("output_dic", "Output filepath of trie dictionary");
    p.add("trie_type", "Trie type: [7|8|15|16] (default=8)", "-t", false);
    p.add("binary_mode", "Is binary mode? (default=0)", "-b", false);
    return p;
}

std::vector<std::string> load_strings(const std::string& filepath, char delim = '\n') {
    std::ifstream ifs(filepath);
    XCDAT_THROW_IF(!ifs.good(), "Cannot open the input file");

    std::vector<std::string> strs;
    for (std::string str; std::getline(ifs, str, delim);) {
        strs.push_back(str);
    }
    return strs;
}

template <class Trie>
int build(const cmd_line_parser::parser& p) {
    const auto input_keys = p.get<std::string>("input_keys");
    const auto output_dic = p.get<std::string>("output_dic");
    const auto binary_mode = p.get<bool>("binary_mode", false);

    auto keys = load_strings(input_keys);
    if (keys.empty()) {
        tfm::errorfln("Error: The input dataset is empty.");
    }

    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    const Trie trie(keys, binary_mode);
    const double memory_in_bytes = xcdat::memory_in_bytes(trie);

    tfm::printfln("Number of keys: %d", trie.num_keys());
    tfm::printfln("Number of trie nodes: %d", trie.num_nodes());
    tfm::printfln("Number of DA units: %d", trie.num_units());
    tfm::printfln("Memory usage in bytes: %d", memory_in_bytes);
    tfm::printfln("Memory usage in MiB: %g", memory_in_bytes / (1024.0 * 1024.0));

    xcdat::save(trie, output_dic);

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

    const auto trie_type = p.get<int>("trie_type", 8);

    switch (trie_type) {
        case 7:
            return build<xcdat::trie_7_type>(p);
        case 8:
            return build<xcdat::trie_8_type>(p);
        case 15:
            return build<xcdat::trie_15_type>(p);
        case 16:
            return build<xcdat::trie_16_type>(p);
        default:
            break;
    }

    p.help();
    return 1;
}