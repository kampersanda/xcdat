#include <iostream>
#include <string>

#include <xcdat.hpp>

int main() {
    // Input keys
    std::vector<std::string> keys = {
        "AirPods",  "AirTag",  "Mac",  "MacBook", "MacBook_Air", "MacBook_Pro",
        "Mac_Mini", "Mac_Pro", "iMac", "iPad",    "iPhone",      "iPhone_SE",
    };

    // The input keys must be sorted and unique (although they have already satisfied in this case).
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    const char* index_filename = "tmp.idx";

    // The trie index type
    using trie_type = xcdat::trie_8_type;

    // Build and save the trie index.
    {
        const trie_type trie(keys);
        xcdat::save(trie, index_filename);
    }

    // Load the trie index.
    const auto trie = xcdat::load<trie_type>(index_filename);

    // Lookup
    {
        const auto id = trie.lookup("Mac_Pro");
        std::cout << "lookup(Mac_Pro) = " << id.value_or(UINT64_MAX) << std::endl;
    }
    {
        const auto id = trie.lookup("Google_Pixel");
        std::cout << "lookup(Google_Pixel) = " << id.value_or(UINT64_MAX) << std::endl;
    }

    // Decoding
    {
        const auto dec = trie.decode(4);
        std::cout << "decode(4) = " << dec << std::endl;
    }

    // Common prefix search
    {
        std::cout << "common_prefix_search(MacBook_Air) = {" << std::endl;
        auto itr = trie.make_prefix_iterator("MacBook_Air");
        while (itr.next()) {
            std::cout << "   (" << itr.decoded_view() << ", " << itr.id() << ")," << std::endl;
        }
        std::cout << "}" << std::endl;
    }

    // Predictive search
    {
        std::cout << "predictive_search(Mac) = {" << std::endl;
        auto itr = trie.make_predictive_iterator("Mac");
        while (itr.next()) {
            std::cout << "   (" << itr.decoded_view() << ", " << itr.id() << ")," << std::endl;
        }
        std::cout << "}" << std::endl;
    }

    // Enumerate all the keys in the trie (in lex order).
    {
        std::cout << "enumerate() = {" << std::endl;
        auto itr = trie.make_enumerative_iterator();
        while (itr.next()) {
            std::cout << "   (" << itr.decoded_view() << ", " << itr.id() << ")," << std::endl;
        }
        std::cout << "}" << std::endl;
    }

    std::remove(index_filename);

    return 0;
}
