#undef NDEBUG

#include <algorithm>
#include <cassert>
#include <random>

#include "DacBc.hpp"
#include "FastDacBc.hpp"

using namespace xcdat;

namespace {

constexpr size_t kSize = (1U << 16);
constexpr uint32_t kUpper = (1U << 31) - 1;

std::vector<BcElement> make_bc() {
  std::random_device rnd;
  std::vector<BcElement> ret;

  for (size_t i = 0; i < kSize; ++i) {
    BcElement item;
    item.base = rnd() % kUpper;
    item.check = rnd() % kUpper;
    switch (rnd() % 3) {
      case 0: // internal node
        item.is_used = true;
        break;
      case 1: // leaf node
        item.is_leaf = true;
        item.is_used = true;
        break;
      case 2: // free node
        break;
      default:
        break;
    }
    ret.push_back(item);
  }

  return ret;
}

template <typename Bc>
void test_bc(const std::vector<BcElement>& orig_bc) {
  Bc bc{orig_bc};

  assert(bc.num_nodes() == orig_bc.size());
  for (uint32_t i = 0; i < orig_bc.size(); ++i) {
    assert(bc.is_used(i) == orig_bc[i].is_used);
    if (!bc.is_used(i)) {
      continue;
    }
    assert(bc.check(i) == orig_bc[i].check);
    assert(bc.is_leaf(i) == orig_bc[i].is_leaf);
    if (!bc.is_leaf(i)) {
      assert(bc.base(i) == orig_bc[i].base);
    } else {
      assert(bc.link(i) == orig_bc[i].base);
    }
  }
}

} // namespace

int main() {
  auto orig_bc = make_bc();

  test_bc<DacBc>(orig_bc);
  test_bc<FastDacBc>(orig_bc);

  return 0;
}
