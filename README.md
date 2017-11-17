# Xcdat: xor-compressed double-array trie

Xcdat is a C++ library that implements static compressed string dictionaries based on an improved double-array trie.

The double array is known as the fastest trie representation and has been used in many trie libraries. On the other hand, it has a space efficiency problem because of a pointer-based data structure. Xcdat solves the problem using the XOR-compressed double-array methods described in the following article.

> Shunsuke Kanda, Kazuhiro Morita, and Masao Fuketa. Compressed double-array tries for string dictionaries supporting fast lookup. Knowledge and Information Systems, 51(3): 1023–1042, 2017. [[doi](https://doi.org/10.1007/s10115-016-0999-8)] [[pdf](https://sites.google.com/site/shnskknd/KAIS2016.pdf)]

Xcdat can implement trie dictionaries in smaller space compared to the other double-array libraries. In addition, the lookup speed is relatively fast in compressed data structures from the double-array advantage.

## Documentation

The document of Xcdat is [here](https://kampersanda.github.io/xcdat/).

## Build Instructions

You can download and compile Xcdat as the following commands.

```
$ git clone https://github.com/kampersanda/xcdat.git
$ cd xcdat
$ mkdir build
$ cd build
$ cmake ..
$ make
$ make install
```
