#include <iostream>
#include <string>

#include <xcdat.hpp>

int main() {
    // Dataset of keywords
    std::vector<std::string> keys = {
        "AirPods",  "AirTag",  "Mac",  "MacBook", "MacBook_Air", "MacBook_Pro",
        "Mac_Mini", "Mac_Pro", "iMac", "iPad",    "iPhone",      "iPhone_SE",
    };

    // The input keys must be sorted and unique (although they have already satisfied in this case).
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    // The trie dictionary type
    using trie_type = xcdat::trie_7_type;
    // using trie_type = xcdat::trie_8_type;
    // using trie_type = xcdat::trie_15_type;
    // using trie_type = xcdat::trie_16_type;

    // The dictionary filename
    const char* tmp_filename = "dic.bin";

    // Build and save the trie dictionary.
    try {
        const trie_type trie(keys);
        xcdat::save(trie, tmp_filename);
    } catch (const xcdat::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    // Load the trie dictionary on memory.
    const auto trie = xcdat::load<trie_type>(tmp_filename);

    // Basic statistics
    std::cout << "Number of keys: " << trie.num_keys() << std::endl;
    std::cout << "Number of trie nodes: " << trie.num_nodes() << std::endl;
    std::cout << "Number of DA units: " << trie.num_units() << std::endl;
    std::cout << "Memory usage in bytes: " << xcdat::memory_in_bytes(trie) << std::endl;

    // Lookup the ID for a query key.
    {
        const auto id = trie.lookup("Mac_Pro");
        std::cout << "Lookup(Mac_Pro) = " << id.value_or(UINT64_MAX) << std::endl;
    }
    {
        const auto id = trie.lookup("Google_Pixel");
        std::cout << "Lookup(Google_Pixel) = " << id.value_or(UINT64_MAX) << std::endl;
    }

    // Decode the key for a query ID.
    {
        const auto dec = trie.decode(4);
        std::cout << "Decode(4) = " << dec << std::endl;
    }

    // Common prefix search
    {
        std::cout << "CommonPrefixSearch(MacBook_Air) = {" << std::endl;
        auto itr = trie.make_prefix_iterator("MacBook_Air");
        while (itr.next()) {
            std::cout << "   (" << itr.decoded_view() << ", " << itr.id() << ")," << std::endl;
        }
        std::cout << "}" << std::endl;
    }

    // Predictive search
    {
        std::cout << "PredictiveSearch(Mac) = {" << std::endl;
        auto itr = trie.make_predictive_iterator("Mac");
        while (itr.next()) {
            std::cout << "   (" << itr.decoded_view() << ", " << itr.id() << ")," << std::endl;
        }
        std::cout << "}" << std::endl;
    }

    // Enumerate all the keys (in lex order).
    {
        std::cout << "Enumerate() = {" << std::endl;
        auto itr = trie.make_enumerative_iterator();
        while (itr.next()) {
            std::cout << "   (" << itr.decoded_view() << ", " << itr.id() << ")," << std::endl;
        }
        std::cout << "}" << std::endl;
    }

    std::remove(tmp_filename);

    return 0;
}
