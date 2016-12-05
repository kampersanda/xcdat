# Xcdat: xor-compressed double-array trie

Xcdat is a C++ library that implements a static compressed dictionary using an improved double-array trie.

The double-array (Aoe, 1989) is known as the fastest trie representation and has been used in many trie libraries. On the other hand, it has a problem for space efficiency because of a pointer-based data structure.
Xcdat solves the problem by applying the XOR-compressed double-array (XCDA) methods described in

- S. Kanda, K. Morita, and M. Fuketa. Compressed double-array tries for string dictionaries supporting fast lookup. _Knowledge and Information Systems_, Online first. [[doi](http://dx.doi.org/10.1007/s10115-016-0999-8)] [[pdf](https://sites.google.com/site/shnskknd/kais2016.pdf)]

Therefore, Xcdat can implement a trie dictionary in smaller space than the other double-array libraries.
In addition, the lookup speed is fast in compressed data structures from the double-array advantage.

## Features

- **Compressed data structure**. Xcdat practically compresses double-array elements representing node pointers by using XCDA methods. While the original double-array uses 8 bytes per node, it uses about 3 ~ 4 bytes (but, depending on a dataset). In addition, the dictionary is implemented by using a minimal-prefix trie unifying suffix strings (Yata et al., 2007), which is effective for long strings in time and space.

- **Two compression versions**. There are two versions for compressing elements: using byte-oriented DACs (Brisaboa et al., 2013) and using pointer-based ones. For characterless strings such as natural language keywords, the former will be slightly smaller and the latter will be slightly faster. For long strings such as URLs, the latter will outperform the former. Xcdat implements the two versions by using a static polymorphism with C++ template to avoid an overhead of virtual functions. 

- **Dictionary coding**. Xcdat supports mapping N different strings to unique IDs in [0,N). That is to say, it supports two basic dictionary operations: Lookup returns the ID corresponding to a given string and Access (also called ReverseLookup) returns the string corresponding to a given ID. Therefore, Xcdat is very useful in many applications for string precessing and indexing, such as described in (Martínez-Prieto et al., 2016).

- **Prefix-based lookup operations**. As with other trie libraries, Xcdat also provides prefix-based lookup operations, which are necessary for natural language processing.

- **Fast operations**. Xcdat can provide lookup operations faster than other compressed trie libraries because it is based on the double-array trie. On the other hand, compared to the existing double-array libraries, the speed is slower due to the compression.

## Build instructions

You can download and compile Xcdat as the following commands:

```
$ git clone https://github.com/kamp78/xcdat.git
$ cd xcdat
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Future work

- Creating API documents
- Showing benchmarks
- Implementing faster operations
- Supporting 64-bit integers
- Clearing up source codes

## References

- J. Aoe. An efficient digital search algorithm by using a double-array structure. _IEEE Transactions on Software Engineering_, 15(9):1066–1077, 1989.
- N. R. Brisaboa, S. Ladra, and G. Navarro. DACs: Bringing direct access to variable-length codes. _Information Processing & Management_, 49(1):392–404, 2013.
- S. Kanda, K. Morita, and M. Fuketa. Compressed double-array tries for string dictionaries supporting fast lookup. _Knowledge and Information Systems_, Online first.
- M. A. Martínez-Prieto, N. Brisaboa, R. Cánovas, F. Claude, and G. Navarro. Practical compressed string dictionaries. _Information Systems_, 56:73–108, 2016.
- S. Yata, M. Oono, K. Morita, M. Fuketa, T. Sumitomo, and J. Aoe. A compact static double-array keeping character codes. _Information Processing & Management_, 43(1):237–247, 2007.
