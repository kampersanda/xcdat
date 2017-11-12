#undef NDEBUG

#include <cassert>
#include <iostream>
#include <random>

#include "xcdat/BitVector.hpp"
#include "xcdat/FitVector.hpp"

using namespace xcdat;

namespace {

constexpr size_t SIZE = 1U << 10;

void test_vector() {
  std::vector<int> orig_vec(SIZE);
  {
    std::random_device rnd;
    for (size_t i = 0; i < SIZE; ++i) {
      orig_vec[i] = rnd();
    }
  }

  auto copied_vec = orig_vec; // copy
  Vector<int> vec(copied_vec);

  assert(copied_vec.empty());

  for (size_t i = 0; i < SIZE; ++i) {
    assert(orig_vec[i] == vec[i]);
  }

  Vector<int> swapped_vec;
  vec.swap(swapped_vec);

  assert(vec.is_empty());

  for (size_t i = 0; i < SIZE; ++i) {
    assert(orig_vec[i] == swapped_vec[i]);
  }
}

void test_bit_vector() {
  std::vector<bool> orig_bit_vector;
  {
    std::random_device rnd;
    for (size_t i = 0; i < SIZE; ++i) {
      orig_bit_vector.push_back(rnd() % 2 == 0);
    }
  }

  BitVector bit_vector;
  {
    BitVectorBuilder builder;
    for (size_t i = 0; i < SIZE; ++i) {
      builder.push_back(orig_bit_vector[i]);
    }
    bit_vector = BitVector(builder, true, true);
  }

  assert(bit_vector.size() == SIZE);

  id_type sum = 0;
  for (id_type i = 0; i < SIZE; ++i) {
    assert(bit_vector[i] == orig_bit_vector[i]);
    if (bit_vector[i]) {
      assert(sum == bit_vector.rank(i));
      assert(i == bit_vector.select(sum));
      ++sum;
    }
  }

  assert(bit_vector.num_1s() == sum);
  assert(bit_vector.num_0s() == SIZE - sum);
}

void test_small_vector() {
  std::vector<id_type> orig_vector;
  {
    std::random_device rnd;
    for (size_t i = 0; i < SIZE; ++i) {
      orig_vector.push_back(rnd() & UINT16_MAX);
    }
  }

  FitVector small_vector(orig_vector);
  assert(orig_vector.size() == small_vector.size());

  for (size_t i = 0; i < SIZE; ++i) {
    assert(orig_vector[i] == small_vector[i]);
  }
}

} // namespace

int main() {
  test_vector();
  test_bit_vector();
  test_small_vector();
  return 0;
}
