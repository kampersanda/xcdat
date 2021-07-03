# Xcdat: Fast compressed trie dictionary library

**Xcdat** is a C++17 header-only library of a fast compressed string dictionary based on an improved double-array trie structure described in the paper: [Compressed double-array tries for string dictionaries supporting fast lookup](https://doi.org/10.1007/s10115-016-0999-8), *Knowledge and Information Systems*, 2017, available at [here](https://kampersanda.github.io/pdf/KAIS2017.pdf).

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
- **Fast and compact data structure.** Xcdat employs the *double-array trie* [3] known as the fastest trie implementation. However, the double-array trie resorts to many pointers and consumes a large amount of memory. To address this, Xcdat applies the *XCDA* method [2] that represents the double-array trie in a compressed format while maintaining the fast searches.
- **Cache efficiency.** Xcdat employs a *minimal-prefix trie* [4] that replaces redundant trie nodes into strings to reduce random access and to improve locality of references.
- **Dictionary encoding.** Xcdat maps `N` distinct keywords into unique IDs from `[0,N-1]`, and supports the two symmetric operations: `lookup` returns the ID corresponding to a given keyword; `decode` returns the keyword associated with a given ID. The mapping is so-called *dictionary encoding* (or *domain encoding*) and is fundamental in many DB applications as described by Martínez-Prieto et al [1] or Müller et al. [5].
- **Prefix search operations.** Xcdat supports prefix search operations realized by trie search algorithms: `prefix_search` returns all the keywords contained as prefixes of a given string; `predictive search` returns all the keywords starting with a given string. These will be useful in many NLP applications such as auto completions [6], stemmed searches [7], or input method editors [8].
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

 Xcdat provides command line tools to build the dictionary and perform searches, which are inspired by [marisa-trie](https://github.com/s-yata/marisa-trie). All the tools will print the command line options by specifying the parameter `-h`.

The tools employ the external libraries [cmd_line_parser](https://github.com/jermp/cmd_line_parser), [mm_file](https://github.com/jermp/mm_file), and [tinyformat](https://github.com/c42f/tinyformat), which are contained in the repository.

### `xcdat_build`

It builds the trie dictionary from a given dataset consisting of keywords separated by newlines. The following command builds the trie dictionary from dataset `enwiki-titles.txt` and writes the dictionary into file `dic.bin`.

```
$ xcdat_build enwiki-titles.txt dic.bin
Number of keys: 15955763
Number of trie nodes: 36439320
Number of DA units: 36515840
Memory usage in bytes: 1.64104e+08
Memory usage in MiB: 156.502
```

### `xcdat_lookup`

It tests the `lookup` operation for a given dictionary. Given a query string via `stdin`, it prints the associated ID if found, or `-1` otherwise.

```
$ xcdat_lookup dic.bin
Algorithm
1255938	Algorithm
Double_Array
-1	Double_Array
```

### `xcdat_decode`

It tests the `decode` operation for a given dictionary. Given a query ID via `stdin`, it prints the corresponding keyword if the ID is in the range `[0,N-1]`, where `N` is the number of stored keywords.

```
$ xcdat_decode dic.bin
1255938
1255938	Algorithm
```

### `xcdat_prefix_search`

It tests the `prefix_search` operation for a given dictionary. Given a query string via `stdin`, it prints all the keywords contained as prefixes of a given string.

```
$ xcdat_prefix_search dic.bin
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

It tests the `predictive_search` operation for a given dictionary. Given a query string via `stdin`, it prints the first `n` keywords starting with a given string, where `n` is one of the parameters.

```
$ xcdat_predictive_search dic.bin -n 3
Algorithm
263 found
1255938	Algorithm
1255944	Algorithm's_optimality
1255972	Algorithm_(C++)
```

### `xcdat_enumerate`

It prints all the keywords stored in a given dictionary.

```
$ xcdat_enumerate dic.bin | head -3
0	!
107	!!
138	!!!
```

### `xcdat_benchmark`

Xcdat provides the four dictionary types defined in `xcdat.hpp`. The tool measures the performances of them for a given dataset. To perform search operations, it randomly samples `n` queires from the dataset, where `n` is one of the parameters. It will help you determine the dictionary type.

```
$ xcdat_benchmark enwiki-titles.txt
** xcdat::trie_7_type **
Number of keys: 15955763
Memory usage in bytes: 1.70618e+08
Memory usage in MiB: 162.714
Construction time in seconds: 13.501
Lookup time in microsec/query: 0.5708
Decode time in microsec/query: 1.0846
** xcdat::trie_8_type **
Number of keys: 15955763
Memory usage in bytes: 1.64104e+08
Memory usage in MiB: 156.502
Construction time in seconds: 13.626
Lookup time in microsec/query: 0.6391
Decode time in microsec/query: 1.0531
** xcdat::trie_15_type **
Number of keys: 15955763
Memory usage in bytes: 2.05737e+08
Memory usage in MiB: 196.206
Construction time in seconds: 13.425
Lookup time in microsec/query: 0.3613
Decode time in microsec/query: 0.7044
** xcdat::trie_16_type **
Number of keys: 15955763
Memory usage in bytes: 2.15935e+08
Memory usage in MiB: 205.932
Construction time in seconds: 13.704
Lookup time in microsec/query: 0.3832
Decode time in microsec/query: 0.8362
```

## Sample usage

`sample/sample.cpp` provides a sample usage.

```c++
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
```

The output will be

```
Number of keys: 12
Number of trie nodes: 28
Number of DA units: 256
Memory usage in bytes: 1766
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

Xcdat consists of only the header files and can be used by including only the header `xcdat.hpp`. Also, it uses `namespace xcdat`.

### Trie dictionary types

The four specialization types of class `xcdat::trie` are provided in `xcdat.hpp`. The first two types are based on standard DACs by Brisaboa et al. [9]. The last two types are based on pointer-based DACs by Kanda et al. [2].

```c++
//! The trie type with standard DACs using 8-bit integers
using trie_8_type = trie<bc_vector_8>;

//! The trie type with standard DACs using 16-bit integers
using trie_16_type = trie<bc_vector_16>;

//! The trie type with pointer-based DACs using 7-bit integers (for the 1st layer)
using trie_7_type = trie<bc_vector_7>;

//! The trie type with pointer-based DACs using 15-bit integers (for the 1st layer)
using trie_15_type = trie<bc_vector_15>;
```

### Trie dictionary class

The trie dictionary class provides the following functions.

```c++
//! A compressed string dictionary based on an improved double-array trie.
//! 'BcVector' is the data type of Base and Check vectors.
template <class BcVector>
class trie {
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
    //!
    //! If bin_mode = false, the NULL character is used for the termination of a keyword.
    //! If bin_mode = true, bit flags are used istead, and the keywords can contain NULL characters.
    //! If the input keywords contain NULL characters, bin_mode will be forced to be set to true.
    //!
    //! The type 'Strings' and 'Strings::value_type' should be a random iterable container such as std::vector.
    //! Precisely, they should support the following operations:
    //!  - size() returns the container size.
    //!  - operator[](i) accesses the i-th element.
    //!  - begin() returns the iterator to the beginning.
    //!  - end() returns the iterator to the end.
    //! The type 'Strings::value_type::value_type' should be one-byte integer type such as 'char'.
    template <class Strings>
    trie(const Strings& keys, bool bin_mode = false);

    //! Check if the binary mode.
    bool bin_mode() const;

    //! Get the number of stored keywords.
    std::uint64_t num_keys() const;

    //! Get the alphabet size.
    std::uint64_t alphabet_size() const;

    //! Get the maximum length of keywords.
    std::uint64_t max_length() const;

    //! Get the number of trie nodes.
    std::uint64_t num_nodes() const;

    //! Get the number of DA units.
    std::uint64_t num_units() const;

    //! Get the number of unused DA units.
    std::uint64_t num_free_units() const;

    //! Get the length of TAIL vector.
    std::uint64_t tail_length() const;

    //! Lookup the ID of the keyword.
    std::optional<std::uint64_t> lookup(std::string_view key) const;

    //! Decode the keyword associated with the ID.
    std::string decode(std::uint64_t id) const;

    //! Decode the keyword associated with the ID and store it in 'decoded'.
    //! It can avoid reallocation of memory to store the result.
    void decode(std::uint64_t id, std::string& decoded) const;

    //! An iterator class for common prefix search.
    //! It enumerates all the keywords contained as prefixes of a given string.
    //! It should be instantiated via the function 'make_prefix_iterator'.
    class prefix_iterator {
      public:
        prefix_iterator() = default;

        //! Increment the iterator.
        //! Return false if the iteration is terminated.
        bool next();

        //! Get the result ID.
        std::uint64_t id() const;

        //! Get the result keyword.
        std::string decoded() const;

        //! Get the reference to the result keyword.
        //! Note that the referenced data will be changed in the next iteration.
        std::string_view decoded_view() const;
    };

    //! Make the common prefix searcher for the given keyword.
    prefix_iterator make_prefix_iterator(std::string_view key) const;

    //! Preform common prefix search for the keyword.
    void prefix_search(std::string_view key, const std::function<void(std::uint64_t, std::string_view)>& fn) const;

    //! An iterator class for predictive search.
    //! It enumerates all the keywords starting with a given string.
    //! It should be instantiated via the function 'make_predictive_iterator'.
    class predictive_iterator {
      public:
        predictive_iterator() = default;

        //! Increment the iterator.
        //! Return false if the iteration is terminated.
        bool next();

        //! Get the result ID.
        std::uint64_t id() const;

        //! Get the result keyword.
        std::string decoded() const;

        //! Get the reference to the result keyword.
        //! Note that the referenced data will be changed in the next iteration.
        std::string_view decoded_view() const;
    };

    //! Make the predictive searcher for the keyword.
    predictive_iterator make_predictive_iterator(std::string_view key) const;

    //! Preform predictive search for the keyword.
    void predictive_search(std::string_view key, const std::function<void(std::uint64_t, std::string_view)>& fn) const;

    //! An iterator class for enumeration.
    //! It enumerates all the keywords stored in the trie.
    //! It should be instantiated via the function 'make_enumerative_iterator'.
    using enumerative_iterator = predictive_iterator;

    //! An iterator class for enumeration.
    enumerative_iterator make_enumerative_iterator() const;

    //! Enumerate all the keywords and their IDs stored in the trie.
    void enumerate(const std::function<void(std::uint64_t, std::string_view)>& fn) const;

    //! Visit the members (commonly used for I/O).
    template <class Visitor>
    void visit(Visitor& visitor);
};
```

### I/O handlers

`xcdat.hpp` provides some functions for handling I/O operations.

```c++
//! Set the continuous memory block to a new trie instance (for a memory-mapped file).
template <class Trie>
Trie mmap(const char* address);

//! Load the trie dictionary from the file.
template <class Trie>
Trie load(const std::string& filepath);

//! Save the trie dictionary to the file and returns the file size in bytes.
template <class Trie>
std::uint64_t save(const Trie& idx, const std::string& filepath);

//! Get the dictionary size in bytes.
template <class Trie>
std::uint64_t memory_in_bytes(const Trie& idx);

//! Get the flag indicating the trie type, embedded by the function 'save'.
//! The flag corresponds to trie::l1_bits and will be used to detect the trie type from the file.
std::uint32_t get_flag(const std::string& filepath);

//! Load the keywords from the file.
std::vector<std::string> load_strings(const std::string& filepath, char delim = '\n');
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

1. M. A. Martínez-Prieto, N. Brisaboa, R. Cánovas, F. Claude, and G. Navarro. Practical compressed string dictionaries. Information Systems, 56:73–108, 2016
2. S. Kanda, K. Morita, and M. Fuketa. Compressed double-array tries for string dictionaries supporting fast lookup. Knowledge and Information Systems, 51(3): 1023–1042, 2017.
3. J. Aoe. An efficient digital search algorithm by using a double-array structure. IEEE Transactions on Software Engineering, 15(9):1066–1077, 1989.
4. S. Yata, M. Oono, K. Morita, M. Fuketa, T. Sumitomo, and J. Aoe. A compact static double-array keeping character codes. Information Processing & Management, 43(1):237–247, 2007.
5. Müller, Ingo, Cornelius Ratsch, and Franz Faerber. Adaptive string dictionary compression in in-memory column-store database systems. In EDBT, pp. 283–294, 2014.
6. Gog, Simon, Giulio Ermanno Pibiri, and Rossano Venturini. Efficient and effective query auto-completion. In SIGIR, pp. 2271–2280, 2020.
7. Ricardo Baeza-Yates, and Berthier Ribeiro-Neto. Modern Information Retrieval. 2nd ed. Addison Wesley, Boston, MA, USA, 2011.
8. Kudo, Taku, et al. Efficient dictionary and language model compression for input method editors. In WTIM, pp. 19–25, 2011.
9. N. R. Brisaboa, S. Ladra, and G. Navarro. DACs: Bringing direct access to variable-length codes. Information Processing & Management, 49(1):392–404, 2013.

