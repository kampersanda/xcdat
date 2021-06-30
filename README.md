# Xcdat: Fast compressed trie dictionary library

**Xcdat** is a C++17 header-only library of a fast compressed string dictionary based on the improved double-array trie structure described in the paper: [Compressed double-array tries for string dictionaries supporting fast lookup](https://doi.org/10.1007/s10115-016-0999-8), *Knowledge and Information Systems*, 2017, available at [here](https://kampersanda.github.io/pdf/KAIS2017.pdf).

## Table of contents

- [Features](#features)
- [Build instructions](#build-instructions)
- [Command line tools](#command-line-tools)
- [Sample usage](#sample-usage)
- [API](#api)
- [Performance](#performance)
- [Licensing](#licensing)
- [Todo](#todo)
- [References](#references)

## Features

- **Compressed string dictionary.** Xcdat implements a (static) *compressed string dictioanry* that stores a set of strings (or keywords) in a compressed space while supporting several search operations [1,2]. For example, Xcdat can store an entire set of English Wikipedia titles at half the size of the raw data.
- **Fast and compact data structure.** Xcdat employs the *double-array trie* [3] known as the fastest data structure for trie implementation. However, the double-array trie resorts to many pointers and consumes a large amount of memory. To address this, Xcdat applies the *XCDA* method [2] that represents the double-array trie in a compressed format while maintaining the fast searches.
- **Cache efficiency.** Xcdat employs a *minimal-prefix trie* [4] that replaces redundant trie nodes into strings, resulting in reducing random access and improving locality of references.
- **Dictionary encoding.** Xcdat maps `N` distinct keywords into unique IDs from `[0,N-1]`, and supports the two symmetric operations: `lookup` returns the ID corresponding to a given keyword; `decode` returns the keyword associated with a given ID. The mapping is so-called *dictionary encoding* (or *domain encoding*) and is fundamental in many DB applications as described by Martínez-Prieto et al [1] or Müller et al. [5].
- **Prefix search operations.** Xcdat supports prefix search operations realized by trie search algorithms: `prefix_search` returns all the keywords contained as prefixes of a given string; `predictive search` returns all the keywords starting with a given string. These will be useful in many NLP applications such as auto completions [6], stemmed searches [7], or morphological analysis [8].
- **64-bit support.** As mentioned before, since the double array is a pointer-based data structure, most double-array libraries use 32-bit pointers to reduce memory consumption, resulting in limiting the scale of the input dataset. On the other hand, the XCDA method allows Xcdat to represent 64-bit pointers without sacrificing memory efficiency.
- **Binary key support.** In normal mode, Xcdat will use the `\0` character as an end marker for each keyword. However, if the dataset include `\0` characters, it will use bit flags instead of end markers, allowing the dataset to consist of binary keywords.
- **Memory mapping.** Xcdat supports *memory mapping*, allowing data to be deserialized quickly without loading it into memory. Of course, deserialization by the loading is also supported.
- **Header only.** The library consists only of header files, and you can easily install it.

## Build instructions

You can download, compile, and install Xcdat with the following commands.

```
$ git clone https://github.com/kampersanda/xcdat.git
$ cd xcdat
$ mkdir build
$ cd build
$ cmake ..
$ make -j
$ make install
```

Or, since this library consists only of header files, you can easily install it by passing through the path to the directory `include`.

### Requirements

You need to install a modern C++17 ready compiler such as `g++ >= 7.0` or `clang >= 4.0`. For the build system, you need to install `CMake >= 3.0` to compile the library.

The library considers a 64-bit operating system. The code has been tested only on Mac OS X and Linux. That is, this library considers only UNIX-compatible OS.

## Command line tools

 Xcdat provides command line tools to build the index and perform searches, which are inspired by [marisa-trie](https://github.com/s-yata/marisa-trie). All the tools will print the command line options by specifying the parameter `-h`.

### `xcdat_build`

It builds the trie index from a given dataset consisting of keywords separated by newlines. The keywords have to be sorted (in ascii order) and unique.

The following command builds the trie index from dataset `enwiki-latest-all-titles-in-ns0` and writes the index into file `idx.bin`.

```
$ xcdat_build enwiki-latest-all-titles-in-ns0 idx.bin -u 1
time_in_sec: 13.449
memory_in_bytes: 1.70618e+08
memory_in_MiB: 162.714
number_of_keys: 15955763
alphabet_size: 198
max_length: 253
```

### `xcdat_lookup`

It tests the `lookup` operation for a given index. Given a query string via `stdin`, it prints the associated ID if found, or `-1` otherwise.

```
$ xcdat_lookup idx.bin
Algorithm
1255938	Algorithm
Double_Array
-1	Double_Array
```

### `xcdat_decode`

It tests the `decode` operation for a given index. Given a query ID via `stdin`, it prints the corresponding keyword if the ID is in the range `[0,N-1]`, where `N` is the number of stored keywords.

```
$ xcdat_decode idx.bin
1255938
1255938	Algorithm
```

### `xcdat_prefix_search`

It tests the `prefix_search` operation for a given index. Given a query string via `stdin`, it prints all the keywords contained as prefixes of a given string.

```
$ xcdat_prefix_search idx.bin
Algorithmic
6 found
57	A
798460	Al
1138004	Alg
1253024	Algo
1255938	Algorithm
1255931	Algorithmic
```

### `xcdat_predictive_search`

It tests the `predictive_search` operation for a given index. Given a query string via `stdin`, it prints the first `n` keywords starting with a given string, where `n` is one of the parameters.

```
$ xcdat_predictive_search idx.bin -n 3
Algorithm
263 found
1255938	Algorithm
1255944	Algorithm's_optimality
1255972	Algorithm_(C++)
```

### `xcdat_enumerate`

It prints all the keywords stored in a given index.

```
$ xcdat_enumerate idx.bin | head -3
0	!
107	!!
138	!!!
```

## Sample usage

`sample/sample.cpp` provides a sample usage.

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

The output will be

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

## API

`xcdat.hpp` provides

- `xcdat::trie_7_type`: 
- `xcdat::trie_8_type`:

### Dictionary class

```c++
template <class BcVector>
class trie {
  public:
    using trie_type = trie<BcVector>;
    using bc_vector_type = BcVector;

    static constexpr auto l1_bits = bc_vector_type::l1_bits;

  public:
    //! Default constructor
    trie() = default;

    //! Default destructor
    virtual ~trie() = default;

    //! Copy constructor (deleted)
    trie(const trie&) = delete;

    //! Copy constructor (deleted)
    trie& operator=(const trie&) = delete;

    //! Move constructor
    trie(trie&&) noexcept = default;

    //! Move constructor
    trie& operator=(trie&&) noexcept = default;

    //! Build the trie from the input keywords, which are lexicographically sorted and unique.
    //! If bin_mode = false, the NULL character is used for the termination of a keyword.
    //! If bin_mode = true, bit flags are used istead, and the keywords can contain NULL characters.
    //! If the input keywords contain NULL characters, bin_mode will be forced to be set to true.
    template <class Strings>
    explicit trie(const Strings& keys, bool bin_mode = false);

    //! Check if the binary mode.
    inline bool bin_mode() const;

    //! Get the number of stored keywords.
    inline std::uint64_t num_keys() const;

    //! Get the alphabet size.
    inline std::uint64_t alphabet_size() const;

    //! Get the maximum length of keywords.
    inline std::uint64_t max_length() const;

    //! Lookup the ID of the keyword.
    inline std::optional<std::uint64_t> lookup(std::string_view key) const;

    //! Decode the keyword associated with the ID.
    inline std::string decode(std::uint64_t id) const;

    //! An iterator class for common prefix search.
    class prefix_iterator {
      public:
        prefix_iterator() = default;

        //! Increment the iterator.
        //! Return false if the iteration is terminated.
        inline bool next();

        //! Get the result ID.
        inline std::uint64_t id() const;

        //! Get the result keyword.
        inline std::string decoded() const;

        //! Get the reference to the result keyword.
        //! Note that the referenced data will be changed in the next iteration.
        inline std::string_view decoded_view() const;
    };

    //! Make the common prefix searcher for the given keyword.
    inline prefix_iterator make_prefix_iterator(std::string_view key) const;

    //! Preform common prefix search for the keyword.
    inline void prefix_search(std::string_view key, const std::function<void(std::uint64_t, std::string_view)>& fn) const;

    //! An iterator class for predictive search.
    class predictive_iterator {
      public:
        predictive_iterator() = default;

        //! Increment the iterator.
        //! Return false if the iteration is terminated.
        inline bool next();

        //! Get the result ID.
        inline std::uint64_t id() const;

        //! Get the result keyword.
        inline std::string decoded() const;

        //! Get the reference to the result keyword.
        //! Note that the referenced data will be changed in the next iteration.
        inline std::string_view decoded_view() const;
    };

    //! Make the predictive searcher for the keyword.
    inline predictive_iterator make_predictive_iterator(std::string_view key) const {
        return predictive_iterator(this, key);
    }

    //! Preform predictive search for the keyword.
    inline void predictive_search(std::string_view key,
                                  const std::function<void(std::uint64_t, std::string_view)>& fn) const {
        auto itr = make_predictive_iterator(key);
        while (itr.next()) {
            fn(itr.id(), itr.decoded_view());
        }
    }

    //! An iterator class for enumeration.
    using enumerative_iterator = predictive_iterator;

    //! An iterator class for enumeration.
    inline enumerative_iterator make_enumerative_iterator() const;

    //! Enumerate all the keywords and their IDs stored in the trie.
    inline void enumerate(const std::function<void(std::uint64_t, std::string_view)>& fn) const;

    //! Visit the members.
    template <class Visitor>
    void visit(Visitor& visitor);
};
```

### I/O handlers

`xcdat.hpp` provides some functions for handling I/O operations.

```c++
template <class Trie>
Trie mmap(const char* address);
```

## Performance

To be added...

## Licensing

This library is free software provided under the MIT License.

If you use the library in academic settings, please cite the following paper.

```
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

## Todo

- Support other language bindings.
- Add SIMD-ization.

## References

1. J. Aoe. An efficient digital search algorithm by using a double-array structure. IEEE Transactions on Software Engineering, 15(9):1066–1077, 1989.
2. N. R. Brisaboa, S. Ladra, and G. Navarro. DACs: Bringing direct access to variable-length codes. Information Processing & Management, 49(1):392–404, 2013.
3. S. Kanda, K. Morita, and M. Fuketa. Compressed double-array tries for string dictionaries supporting fast lookup. Knowledge and Information Systems, 51(3): 1023–1042, 2017.
4. M. A. Martínez-Prieto, N. Brisaboa, R. Cánovas, F. Claude, and G. Navarro. Practical compressed string dictionaries. Information Systems, 56:73–108, 2016
5. S. Yata, M. Oono, K. Morita, M. Fuketa, T. Sumitomo, and J. Aoe. A compact static double-array keeping character codes. Information Processing & Management, 43(1):237–247, 2007.

