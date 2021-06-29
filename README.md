# Xcdat: Fast compressed trie dictionary library

## What is Xcdat?

Xcdat is a C++17 header-only library that implements static compressed string dictionaries based on an improved double-array trie.

The double array is known as the fastest trie representation and has been used in many trie libraries. On the other hand, it has a space efficiency problem because of a pointer-based data structure. Xcdat solves the problem using the XOR-compressed double-array methods described in the following article.

> Shunsuke Kanda, Kazuhiro Morita, and Masao Fuketa. **Compressed double-array tries for string dictionaries supporting fast lookup.** *Knowledge and Information Systems*, 51(3): 1023–1042, 2017 [doi](https://doi.org/10.1007/s10115-016-0999-8) [pdf](https://kampersanda.github.io/pdf/KAIS2017.pdf)

Xcdat can implement trie dictionaries in smaller space compared to the other double-array libraries. In addition, the lookup speed is relatively fast in compressed data structures from the double-array advantage.

## Features

- **Fast and memory-efficient:** Xcdat employs the double-array structure, known as the fastest trie data structure, and.
- **Compressed data structure**: Xcdat practically compresses double-array elements for representing node pointers by using the XCDA methods. While the original double array uses 8 bytes (or 16 bytes) per node, it uses about 3–4 bytes (but, depending on datasets). In addition, the dictionary is implemented using a minimal-prefix trie (Yata et al., 2007) that is effective for long strings in time and space.
- **Two compression approaches**: There are two approaches of compressing elements: using byte-oriented DACs (Brisaboa et al., 2013) and using pointer-based ones (Kanda et al., 2017). For characterless strings such as natural language keywords, the former will be slightly smaller and the latter will be slightly faster. For long strings such as URLs, the latter will outperform the former. Xcdat implements the two versions by using a static polymorphism with C++ template to avoid an overhead of virtual functions.
- **64-bit Version**: Although Xcdat represents node addresses using 32-bit integers in default configuration, we can allow for 64-bit integers by defining `XCDAT_X64`; therefore, the dictionary can be constructed from a very large dataset. The construction space becomes large, but the output dictionary size is nearly equal.
- **Binary string**: The dictionary can be constructed from keys including the NULL character by setting the second parameter of `xcdat::TrieBuilder::build()` to `true`.
- **Dictionary encoding**: Xcdat supports mapping N different strings to unique IDs in [0,N-1]. That is to say, it supports two basic dictionary operations: Lookup returns the ID corresponding to a given string and Access (also called ReverseLookup) returns the string corresponding to a given ID. Therefore, Xcdat is very useful in many applications for string precessing and indexing, such as described in (Martínez-Prieto et al., 2016).
- **Fast search**: Xcdat can provide lookup operations faster than other compressed trie libraries because it is based on the double-array trie. On the other hand, compared to the existing double-array libraries, the speed will be slower due to the compression.
- **Prefix-based Lookup Operations**: As with other trie libraries, Xcdat also provides prefix-based lookup operations required for natural language processing and so on.

## Build Instructions

You can download and compile Xcdat as the following commands.

```sh
$ git clone https://github.com/kampersanda/xcdat.git
$ cd xcdat
$ mkdir build
$ cd build
$ cmake ..
$ make
$ make install
```

## Command line tools



## Sample

```c++
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

    // Basic statistics
    std::cout << "NumberKeys: " << trie.num_keys() << std::endl;
    std::cout << "MaxLength: " << trie.max_length() << std::endl;
    std::cout << "AlphabetSize: " << trie.alphabet_size() << std::endl;
    std::cout << "Memory: " << xcdat::memory_in_bytes(trie) << " bytes" << std::endl;

    // Lookup IDs from keys
    {
        const auto id = trie.lookup("Mac_Pro");
        std::cout << "Lookup(Mac_Pro) = " << id.value_or(UINT64_MAX) << std::endl;
    }
    {
        const auto id = trie.lookup("Google_Pixel");
        std::cout << "Lookup(Google_Pixel) = " << id.value_or(UINT64_MAX) << std::endl;
    }

    // Decode keys from IDs
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

    std::remove(index_filename);
    return 0;
}
```



```
NumberKeys: 12
MaxLength: 11
AlphabetSize: 20
Memory: 1762 bytes
Lookup(Mac_Pro) = 7
Lookup(Google_Pixel) = 18446744073709551615
Decode(4) = MacBook_Air
CommonPrefixSearch(MacBook_Air) = {
   (Mac, 1),
   (MacBook, 2),
   (MacBook_Air, 4),
}
PredictiveSearch(Mac) = {
   (Mac, 1),
   (MacBook, 2),
   (MacBook_Air, 4),
   (MacBook_Pro, 11),
   (Mac_Mini, 5),
   (Mac_Pro, 7),
}
Enumerate() = {
   (AirPods, 0),
   (AirTag, 3),
   (Mac, 1),
   (MacBook, 2),
   (MacBook_Air, 4),
   (MacBook_Pro, 11),
   (Mac_Mini, 5),
   (Mac_Pro, 7),
   (iMac, 10),
   (iPad, 6),
   (iPhone, 8),
   (iPhone_SE, 9),
}
```



## Licensing

This library is free software provided under the MIT License.

If you use the library in academic settings, please cite the following paper.

```tex
@article{kanda2017compressed,
    title={Compressed double-array tries for string dictionaries supporting fast lookup},
    author={Kanda, Shunsuke and Morita, Kazuhiro and Fuketa, Masao},
    journal={Knowledge and Information Systems (KAIS)},
    volume={51},
    number={3},
    pages={1023--1042},
    year={2017},
    publisher={Springer}
}
```

## References

1. J. Aoe. An efficient digital search algorithm by using a double-array structure. IEEE Transactions on Software Engineering, 15(9):1066–1077, 1989.
2. N. R. Brisaboa, S. Ladra, and G. Navarro. DACs: Bringing direct access to variable-length codes. Information Processing & Management, 49(1):392–404, 2013.
3. S. Kanda, K. Morita, and M. Fuketa. Compressed double-array tries for string dictionaries supporting fast lookup. Knowledge and Information Systems, 51(3): 1023–1042, 2017.
4. M. A. Martínez-Prieto, N. Brisaboa, R. Cánovas, F. Claude, and G. Navarro. Practical compressed string dictionaries. Information Systems, 56:73–108, 2016
5. S. Yata, M. Oono, K. Morita, M. Fuketa, T. Sumitomo, and J. Aoe. A compact static double-array keeping character codes. Information Processing & Management, 43(1):237–247, 2007.

