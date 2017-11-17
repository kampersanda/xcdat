% Xcdat: XOR-compressed double-array trie
% Shunsuke Kanda
% 2017

## What is Xcdat?

Xcdat is a C++ library that implements static compressed string dictionaries based on an improved double-array trie.

The double array (Aoe, 1989) is known as the fastest trie representation and has been used in many trie libraries. On the other hand, it has a space efficiency problem because of a pointer-based data structure. Xcdat solves the problem using the XOR-compressed double-array methods described in the following article.

> Shunsuke Kanda, Kazuhiro Morita, and Masao Fuketa. Compressed double-array tries for string dictionaries supporting fast lookup. Knowledge and Information Systems, 51(3): 1023–1042, 2017. [[doi](https://doi.org/10.1007/s10115-016-0999-8)] [[pdf](https://sites.google.com/site/shnskknd/KAIS2016.pdf)]

Xcdat can implement trie dictionaries in smaller space compared to the other double-array libraries. In addition, the lookup speed is relatively fast in compressed data structures from the double-array advantage.

Xcdat is available at [GitHub repsitory](https://github.com/kampersanda/xcdat).

## Features

- **Compressed Data Structure**: Xcdat practically compresses double-array elements for representing node pointers by using the XCDA methods. While the original double array uses 8 bytes (or 16 bytes) per node, it uses about 3–4 bytes (but, depending on datasets). In addition, the dictionary is implemented using a minimal-prefix trie (Yata et al., 2007) that is effective for long strings in time and space.
- **Two Compression Approaches**: There are two approaches of compressing elements: using byte-oriented DACs (Brisaboa et al., 2013) and using pointer-based ones (Kanda et al., 2017). For characterless strings such as natural language keywords, the former will be slightly smaller and the latter will be slightly faster. For long strings such as URLs, the latter will outperform the former. Xcdat implements the two versions by using a static polymorphism with C++ template to avoid an overhead of virtual functions.
- **64-bit Version**: Although Xcdat represents node addresses using 32-bit integers in default configuration, we can allow for 64-bit integers by defining `XCDAT_X64`; therefore, the dictionary can be constructed from a very large dataset. The construction space becomes large, but the output dictionary size is nearly equal.
- **NULL Character**: The dictionary can be constructed from keys including the NULL character by setting the second parameter of `xcdat::TrieBuilder::build()` to `true`.
- **Dictionary Encoding**: Xcdat supports mapping N different strings to unique IDs in [0,N-1]. That is to say, it supports two basic dictionary operations: Lookup returns the ID corresponding to a given string and Access (also called ReverseLookup) returns the string corresponding to a given ID. Therefore, Xcdat is very useful in many applications for string precessing and indexing, such as described in (Martínez-Prieto et al., 2016).
- **Fast Operations**: Xcdat can provide lookup operations faster than other compressed trie libraries because it is based on the double-array trie. On the other hand, compared to the existing double-array libraries, the speed will be slower due to the compression.
- **Prefix-based Lookup Operations**: As with other trie libraries, Xcdat also provides prefix-based lookup operations required for natural language processing and so on.

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

If you want to use a 64-bit setting, please add `-DXCDAT_X64=ON` to the CMake option. In addition, you can use the SSE4.2 POPCNT instruction by adding `-DXCDAT_USE_POPCNT=ON` for Rank/Select operations. The code has been tested only on Mac OS X and Linux. That is, this library considers only UNIX-compatible OS.


## Command Line Tools

`xcdat` is a general-purpose command line tool to provide three modes as follows.

```
$ xcdat 
xcdat build <type> <key> <dict>
	<type>	1: DACs, 2: FDACs
	<key> 	Input file name of a set of keys (must be sorted)
	<dict>	Output file name of the dictionary (optional)
	      	If omitted, <key>.dacs or <key>.fdacs is output
xcdat query <type> <dict> <limit>
	<type> 	1: DACs, 2: FDACs
	<dict> 	Input file name of the dictionary
	<limit>	Limit of #results (optional, default=10)
xcdat bench <type> <dict> <key>
	<type>	1: DACs, 2: FDACs
	<dict>	Input file name of the dictionary
	<key> 	Input file name of keys for benchmark
```

### Example 1: Construction

Command `xcdat build [params...]` builds Xcdat dictionaries from a given dataset and saves it to a file, as follows.

```
$ xcdat build 1 jawiki-all-titles
constr. time:	1.58574 sec
cmpr. ratio:	0.524287 over the raw size

basic statistics of xcdat::Trie<false>
	num keys:      	1738995
	alphabet size: 	189
	num nodes:     	4042496
	num used nodes:	4034357
	num free nodes:	8139
	size in bytes: 	20546967
member size statistics of xcdat::Trie<false>
	bc:            	13879098	0.675482
	terminal_flags:	708448	0.0344794
	tail:          	5958655	0.290002
	boundary_flags:	40	1.94676e-06
basic statistics of xcdat::DacBc
	num links:     	1499605
	bytes per node:	3.4333
member size statistics of xcdat::DacBc
	values_L0:	8085000	0.582531
	values_L1:	746760	0.0538046
	values_L2:	22581	0.00162698
	flags_L0: 	1389660	0.100126
	flags_L1: 	128400	0.00925132
	leaves:   	694856	0.0500649
	links:    	2811784	0.202591

output -> jawiki-all-titles.dac
```

### Example 2: Query Processing

Command `xcdat query [params...]` loads a dictionary file and tests lookup operations, as follows.

```
$ xcdat query 1 jawiki-all-titles.dac
> NEW_GAME!
Lookup
125989	NEW_GAME!
Common Prefix Lookup
28	N
124185	NE
125428	NEW
125988	NEW_GAME
125989	NEW_GAME!
5 found
Predictive Lookup
125989	NEW_GAME!
126003	NEW_GAME!!
126059	NEW_GAME!_-THE_CHALLENGE_STAGE!-
3 found
```

### Example 3: Benchmark Test

Command `xcdat bench [params...]` tests time performances of a given dictionary, as follows.

```
$ xcdat bench 1 jawiki-all-titles.dac jawiki-all-titles.rnd
Warm up
Lookup benchmark on 10 runs
1.5065 us per str
Access benchmark on 10 runs
1.81289 us per ID
```

## Sample Usage

The following code shows an easy routine sample.

```cpp
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
```

The standard output is as follows.

```
Performing basic operations...
7 : Rin
? : Hotaru
Performing common prefix operations...
4 : Nene
6 : Nenecchi
Performing predictive operations...
3 : Hajime
5 : Hazuki
Enumerating all registered keys...
0 : Aoba
3 : Hajime
5 : Hazuki
1 : Hihumi
2 : Kou
4 : Nene
6 : Nenecchi
7 : Rin
8 : Umiko
9 : Yun
```

As shown in the output, `xcdat::Trie` assigns unique integer IDs to each registered key. The ID order is random, depending on node arrangement.

## API

You can build a dictionary using static member function `xcdat::TrieBuilder::build()`. 
This function receives a set of keywords and returns the resulting class object of `xcdat::Trie`.
For the usage, refer to the header comments of [`xcdat::TrieBuilder.hpp`](https://github.com/kampersanda/xcdat/blob/master/include/xcdat/TrieBuilder.hpp).
Also for the usage of `xcdat::Trie`, refer to the header comments of [`xcdat::Trie`](https://github.com/kampersanda/xcdat/blob/master/include/xcdat/Trie.hpp).

The detailed descriptions of AIP are under construction...

## Benchmark

Work in progress...

## To Do

- Show benchmarks
- Create AIP descriptions

## Licensing

This library is free software provided under the MIT License.

## Citation

If you use the library in academic settings, please cite the following paper.

```bibtex
@article{kanda2017compressed,
	title={Compressed double-array tries for string dictionaries supporting fast lookup},
	author={Kanda, Shunsuke and Morita, Kazuhiro and Fuketa, Masao},
	journal={Knowledge and Information Systems},
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