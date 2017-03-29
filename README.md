# Xcdat: xor-compressed double-array trie

Xcdat is a C++ library that implements static compressed dictionaries based on an improved double-array trie.

The double array (Aoe, 1989) is known as the fastest trie representation and has been used in many trie libraries. On the other hand, it has a space efficiency problem because of a pointer-based data structure.
Xcdat solves the problem using the XOR-compressed double array (XCDA) methods described in

- S. Kanda, K. Morita, and M. Fuketa. Compressed double-array tries for string dictionaries supporting fast lookup. _Knowledge and Information Systems_, Online first. [[doi](http://dx.doi.org/10.1007/s10115-016-0999-8)] [[pdf](https://kamp78.github.io/pdf/KAIS16_preprint.pdf)]

Therefore, Xcdat can implement trie dictionaries in smaller space compared to the other double-array libraries.
In addition, the lookup speed is relatively fast in compressed data structures from the double-array advantage.

## Features

- **Compressed data structure.** Xcdat practically compresses double-array elements for representing node pointers by using the XCDA methods. While the original double array uses 8 bytes (or 16 bytes) per node, it uses about 3 ~ 4 bytes (but, depending on datasets). In addition, the dictionary is implemented using a minimal-prefix trie (Yata et al., 2007) that is effective for long strings in time and space.

- **Two compression versions.** There are two versions for compressing elements: using byte-oriented DACs (Brisaboa et al., 2013) and using pointer-based ones (Kanda et al., 2016). For characterless strings such as natural language keywords, the former will be slightly smaller and the latter will be slightly faster. For long strings such as URLs, the latter will outperform the former. Xcdat implements the two versions by using a static polymorphism with C++ template to avoid an overhead of virtual functions. 

- **64-bit version.** Although Xcdat represents array addresses using 32-bit integers in default configuration, we can allow for 64-bit integers by defining `XCDAT64`; therefore, the dictionary can be constructed from a very large dataset. The construction space becomes large, but the output dictionary size is nearly equal.

- **NULL character.** The dictionary can be constructed from keys including the NULL character by setting the second parameter of the [xcdat::Trie](https://github.com/kamp78/xcdat/blob/master/src/Trie.hpp) constructer to `true`. However, we can generally construct high-performance dictionaries without this setting.

- **Invertible dictionary coding.** Xcdat supports mapping N different strings to unique IDs in [0,N). That is to say, it supports two basic dictionary operations: Lookup returns the ID corresponding to a given string and Access (also called ReverseLookup) returns the string corresponding to a given ID. Therefore, Xcdat is very useful in many applications for string precessing and indexing, such as described in (Martínez-Prieto et al., 2016).

- **Prefix-based lookup operations.** As with other trie libraries, Xcdat also provides prefix-based lookup operations required for natural language processing and so on.

- **Fast operations.** Xcdat can provide lookup operations faster than other compressed trie libraries because it is based on the double-array trie. On the other hand, compared to the existing double-array libraries, the speed will be slower due to the compression.

## Build instructions

You can download and compile Xcdat as the following commands:

```
$ git clone https://github.com/kamp78/xcdat.git
$ cd xcdat
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release -DXCDAT64=OFF
$ make
```

## API

Refer to the header comments of [xcdat::Trie](https://github.com/kamp78/xcdat/blob/master/src/Trie.hpp).

## Benchmark

WIP

## Future work

- Show benchmarks
- Implement faster operations
- Clear up source codes
- Set install opetions

## References

- J. Aoe. An efficient digital search algorithm by using a double-array structure. _IEEE Transactions on Software Engineering_, 15(9):1066–1077, 1989.
- N. R. Brisaboa, S. Ladra, and G. Navarro. DACs: Bringing direct access to variable-length codes. _Information Processing & Management_, 49(1):392–404, 2013.
- S. Kanda, K. Morita, and M. Fuketa. Compressed double-array tries for string dictionaries supporting fast lookup. _Knowledge and Information Systems_, Online first.
- M. A. Martínez-Prieto, N. Brisaboa, R. Cánovas, F. Claude, and G. Navarro. Practical compressed string dictionaries. _Information Systems_, 56:73–108, 2016.
- S. Yata, M. Oono, K. Morita, M. Fuketa, T. Sumitomo, and J. Aoe. A compact static double-array keeping character codes. _Information Processing & Management_, 43(1):237–247, 2007.
