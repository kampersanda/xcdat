#include <iostream>
#include <string>

#include <xcdat.hpp>

int main() {
    std::vector<std::string> keys = {
        "Mac", "MacBook", "MacBook_Air", "MacBook_Pro", "Mac_Pro", "iMac", "Mac_Mini",
    };

    // The dataset must be sorted and unique.
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    auto trie = xcdat::trie::build(keys);

    std::cout << "Basic operations" << std::endl;
    {
        const auto id = trie.lookup("MacBook_Pro");
        if (id.has_value()) {
            std::cout << trie.access(id.value()) << " -> " << id.has_value() << std::endl;
        } else {
            std::cout << "Not found" << std::endl;
        }
    }

    std::cout << "Common prefix search" << std::endl;
    {
        auto itr = trie.make_prefix_iterator("MacBook_Air");
        while (itr.next()) {
            std::cout << itr.prefix() << " -> " << itr.id() << std::endl;
        }
    }

    std::cout << "Predictive search" << std::endl;
    {
        auto itr = trie.make_predictive_iterator("Mac");
        while (itr.next()) {
            std::cout << itr.prefix() << " -> " << itr.id() << std::endl;
        }
    }

    return 0;
}
