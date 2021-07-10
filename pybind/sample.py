#!/usr/bin/env python3

import xcdat
import os

# Prepare the dataset of keywords
keyset = xcdat.Keyset()
keyset.append('AirPods')
keyset.append('AirTag')
keyset.append('Mac')
keyset.append('MacBook')
keyset.append('MacBook_Air')
keyset.append('MacBook_Pro')
keyset.append('Mac_Mini')
keyset.append('Mac_Pro')
keyset.append('iMac')
keyset.append('iPad')
keyset.append('iPhone')
keyset.append('iPhone_SE')
keyset.complete()

# You can choose the trie type from the four types.
Trie = xcdat.Trie8
# Trie = xcdat.Trie16
# Trie = xcdat.Trie7
# Trie = xcdat.Trie15

# Build the trie dictionary
trie = Trie(keyset)

# Get the statistics
print(f'Number of keys: {trie.num_keys()}')
print(f'Number of trie nodes: {trie.num_nodes()}')
print(f'Number of DA units: {trie.num_units()}')
print(f'Memory usage in bytes: {trie.memory_in_bytes()}')

# Lookup the ID for a query key.
print(f'Lookup(Mac_Pro) = {trie.lookup("Mac_Pro")}')
print(f'Lookup(Google_Pixel) = {trie.lookup("Google_Pixel")}')

# Decode the key for a query ID.
print(f'Decode(4) = {trie.decode(4)}')

# Common prefix search
print('CommonPrefixSearch(MacBook_Air) = {')
itr = trie.make_prefix_iterator('MacBook_Air')
while itr.next():
    print(f'  ({itr.decoded()}, {itr.id()})')
print('}')

# Predictive search (in lexicographical order).
print('PredictiveSearch(Mac) = {')
itr = trie.make_predictive_iterator('Mac')
while itr.next():
    print(f'  ({itr.decoded()}, {itr.id()})')
print('}')

# Enumerate all the keys (in lexicographical order).
print('Enumerate() = {')
itr = trie.make_enumerative_iterator()
while itr.next():
    print(f'  ({itr.decoded()}, {itr.id()})')
print('}')

# Save the trie to the file
trie.save('dic.bin')

# Load the trie from the file
other = Trie('dic.bin')
assert trie.num_keys() == other.num_keys()
assert trie.num_nodes() == other.num_nodes()
assert trie.num_units() == other.num_units()
assert trie.memory_in_bytes() == other.memory_in_bytes()

os.remove('dic.bin')
