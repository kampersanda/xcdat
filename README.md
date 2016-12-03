# Xcdat: xor-compressed double-array trie

Xcdat is a C++ library that implements a static dictionary structure based on a double-array trie. The double-array is known as the fastest trie representation and has been used in many trie libraries. On the other hand, it has a problem for space efficiency because of a pointer-based data structure. Xcdat solves the problem by applying the XOR-compressed double-array (XCDA) described on 

- S. Kanda, K. Morita, and M. Fuketa, "Compressed double-array tries for string dictionaries supporting fast lookup", _Knowledge and Information Systems_, Online first. [[pdf](https://sites.google.com/site/shnskknd/kais2016.pdf)]

Therefore, Xcdat can implement a trie dictionary in smaller space than the other double-array libraries.
In addition, the lookup speed is fairly fast in compressed data structures from the double-array advantage.

## Features

- **Compressed data structure**. Xcdat practically compresses double-array elements representing node pointers by using XCDA methods. While the original double-array uses 8 bytes per node, it uses about 3 ~ 4 bytes (but, depending on a dataset). In addition, the dictionary is implemented by using a minimal-prefix trie unifying suffix strings. The structure is effective for long strings in time and space.
- **Two compression versions**. XCDA includes two versions for compressing elements: using byte-oriented directly addressable codes (DACs) and using pointer-based ones. For characterless strings such as natural language keywords, the former will be slightly smaller and the latter will be slightly faster. For long strings such as URLs, the latter will outperform the former. Xcdat implements the two versions by using a static polymorphism with C++ template to avoid an overhead of virtual functions. 
- **Dictionary coding**. Xcdat supports mapping N different strings to unique IDs in [0,N). That is to say, it supports two basic dictionary operations: Lookup return the ID corresponding to a given string and Access (also called ReverseLookup) return the string corresponding to a given ID. Therefore, Xcdat is very useful in many applications for string precessing and indexing.
- **Prefix-based lookup operations**. As with other trie libraries, Xcdat also provides prefix-based lookup operations, which are useful in natural language processing and so on.
- **Fast operations**. The lookup speed of Xcdat is faster than that of other compressed trie libraries because it is based on the double-array trie. On the other hand, compared to the existing double-array libraries, the speed is slower because of the compression.

## Build instructions

You can download and compile the library as the following commands:

```
$ git clone https://github.com/kamp78/xcdat.git
$ cd xcdat
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Todo

- Creating API documents
- Showing benchmarks
- Supporting faster operations
- Supporting 64-bit integers

## Acknowledgements

I would like to thank Dr. Yata, a creator of sophisticated software such as Darts-clone and Marisa-trie, for kindly giving useful comments to a previous version of the library.

