#include <iostream>
#include <string>

#include <xcdat.hpp>

using xcdat_trie = xcdat::trie_8_type;

int main() {
    std::vector<std::string> keys = {
        "AirPods",  "AirTag",  "Mac",  "MacBook", "MacBook_Air", "MacBook_Pro",
        "Mac_Mini", "Mac_Pro", "iMac", "iPad",    "iPhone",      "iPhone_SE",
    };

    // The dataset must be sorted and unique (although it is not needed for the keys).
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    const std::string index_filename = "tmp.idx";

    // Build and save the trie index
    {
        const auto trie = xcdat_trie::build(keys);
        trie.save(index_filename);
    }

    // Load the trie index
    const auto trie = xcdat_trie::load(index_filename);

    std::cout << "Basic operations" << std::endl;
    {
        const auto id = trie.lookup("MacBook_Pro");
        if (id.has_value()) {
            std::cout << trie.decode(id.value()) << " -> " << id.has_value() << std::endl;
        } else {
            std::cout << "Not found" << std::endl;
        }
    }

    std::cout << "Common prefix search" << std::endl;
    {
        auto itr = trie.make_prefix_iterator("MacBook_Air");
        while (itr.next()) {
            std::cout << itr.decoded_view() << " -> " << itr.id() << std::endl;
        }
    }

    std::cout << "Predictive search" << std::endl;
    {
        auto itr = trie.make_predictive_iterator("Mac");
        while (itr.next()) {
            std::cout << itr.decoded_view() << " -> " << itr.id() << std::endl;
        }
    }

    std::remove(index_filename.c_str());

    return 0;
}
