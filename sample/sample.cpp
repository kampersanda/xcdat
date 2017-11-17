#include <iostream>
#include <xcdat.hpp>

int main() {
  std::vector<std::string> keys_buf = {
    "Aoba", "Yun", "Hajime", "Hihumi", "Kou", "Rin",
    "Hazuki", "Umiko", "Nene", "Nenecchi"
  };

  // Convert to the input format
  std::vector<std::string_view> keys(keys_buf.size());
  for (size_t i = 0; i < keys.size(); ++i) {
    keys[i] = std::string_view{keys_buf[i]};
  }

  // Input data must be sorted.
  std::sort(std::begin(keys), std::end(keys));

  // Dictionary class
  using Trie = xcdat::Trie<true>;

  try {
    // Builds a dictionary from the keys
    Trie trie = xcdat::TrieBuilder::build<true>(keys); // move

    // Writes the dictionary to a file.
    std::ofstream ofs{"sample.bin"};
    trie.write(ofs);
  } catch (const xcdat::TrieBuilder::Exception& ex) {
    // Abort if something went wrong...
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  // Creates an empty dictionary
  Trie trie;
  {
    // Reads the dictionary to the file.
    std::ifstream ifs{"sample.bin"};
    trie = Trie{ifs}; // move
  }

  std::cout << "Performing basic operations..." << std::endl;
  {
    // lookup() obtains the unique ID for a given key
    xcdat::id_type key_id = trie.lookup("Rin");
    // access() decodes the key from a given ID
    std::cout << key_id << " : " << trie.access(key_id) << std::endl;

    // Given an unregistered key, lookup() returns NOT_FOUND.
    if (trie.lookup("Hotaru") == Trie::NOT_FOUND) {
      std::cout << "? : " << "Hotaru" << std::endl;
    }
  }

  std::cout << "Performing a common prefix operation..." << std::endl;
  {
    // Common prefix operation is implemented using PrefixIterator, created by
    // make_prefix_iterator().
    Trie::PrefixIterator it = trie.make_prefix_iterator("Nenecchi");

    // next() continues to obtain the next key until false is returned.
    while (it.next()) {
      std::cout << it.id() << " : " << it.key() << std::endl;
    }
  }

  std::cout << "Performing a predictive operation..." << std::endl;
  {
    // Predictive operation is implemented using PredictiveIterator, created by
    // make_predictive_iterator().
    Trie::PredictiveIterator it = trie.make_predictive_iterator("Ha");

    // next() continues to obtain the next key until false is returned in
    // lexicographical order.
    while (it.next()) {
      std::cout << it.id() << " : " << it.key() << std::endl;
    }
  }

  std::cout << "Enumerating all registered keys..." << std::endl;
  {
    // PredictiveIterator for an empty string provides enumeration of all
    // registered keys in lexicographical order.
    Trie::PredictiveIterator it = trie.make_predictive_iterator("");
    while (it.next()) {
      std::cout << it.id() << " : " << it.key() << std::endl;
    }
  }

  return 0;
}
