#undef NDEBUG

#include <cassert>
#include <iostream>
#include <random>

#include "BitVector.hpp"
#include "FitVector.hpp"

using namespace xcdat;

namespace {

constexpr size_t kSize = 1U << 10;

void test_bit_vector() {
  std::vector<bool> orig_bit_vector;
  {
    std::random_device rnd;
    for (size_t i = 0; i < kSize; ++i) {
      orig_bit_vector.push_back(rnd() % 2 == 0);
    }
  }

  BitVector bit_vector;
  {
    BitVectorBuilder builder;
    for (size_t i = 0; i < kSize; ++i) {
      builder.push_back(orig_bit_vector[i]);
    }
    BitVector(builder, true, true).swap(bit_vector);
  }

  assert(bit_vector.size() == kSize);

  size_t sum = 0;
  for (size_t i = 0; i < kSize; ++i) {
    assert(bit_vector[i] == orig_bit_vector[i]);
    if (bit_vector[i]) {
      assert(sum == bit_vector.rank(i));
      assert(i == bit_vector.select(sum));
      ++sum;
    }
  }

  assert(bit_vector.num_1s() == sum);
  assert(bit_vector.num_0s() == kSize - sum);
}

void test_small_vector() {
  std::vector<id_type> orig_vector;
  {
    std::random_device rnd;
    for (size_t i = 0; i < kSize; ++i) {
      orig_vector.push_back(rnd() & UINT16_MAX);
    }
  }

  FitVector small_vector(orig_vector);
  assert(orig_vector.size() == small_vector.size());

  for (size_t i = 0; i < kSize; ++i) {
    assert(orig_vector[i] == small_vector[i]);
  }
}

} // namespace

int main() {
  test_bit_vector();
  test_small_vector();
  return 0;
}
