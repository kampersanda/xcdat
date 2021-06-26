# Xcdat: Fast compressed double-array trie library

## What is Xcdat?

Xcdat is a C++17 library that implements static compressed string dictionaries based on an improved double-array trie.

The double array is known as the fastest trie representation and has been used in many trie libraries. On the other hand, it has a space efficiency problem because of a pointer-based data structure. Xcdat solves the problem using the XOR-compressed double-array methods described in the following article.

> Shunsuke Kanda, Kazuhiro Morita, and Masao Fuketa. **Compressed double-array tries for string dictionaries supporting fast lookup.** *Knowledge and Information Systems*, 51(3): 1023–1042, 2017 [doi](https://doi.org/10.1007/s10115-016-0999-8) [pdf](https://kampersanda.github.io/pdf/KAIS2017.pdf)

Xcdat can implement trie dictionaries in smaller space compared to the other double-array libraries. In addition, the lookup speed is relatively fast in compressed data structures from the double-array advantage.

## Features

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

- J. Aoe. An efficient digital search algorithm by using a double-array structure. IEEE Transactions on Software Engineering, 15(9):1066–1077, 1989.
- N. R. Brisaboa, S. Ladra, and G. Navarro. DACs: Bringing direct access to variable-length codes. Information Processing & Management, 49(1):392–404, 2013.
- S. Kanda, K. Morita, and M. Fuketa. Compressed double-array tries for string dictionaries supporting fast lookup. Knowledge and Information Systems, 51(3): 1023–1042, 2017.
- M. A. Martínez-Prieto, N. Brisaboa, R. Cánovas, F. Claude, and G. Navarro. Practical compressed string dictionaries. Information Systems, 56:73–108, 2016
- S. Yata, M. Oono, K. Morita, M. Fuketa, T. Sumitomo, and J. Aoe. A compact static double-array keeping character codes. Information Processing & Management, 43(1):237–247, 2007.
