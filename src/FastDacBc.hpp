#ifndef XCDAT_FAST_DAC_BC_HPP_
#define XCDAT_FAST_DAC_BC_HPP_

#include <tuple>

#include "BitVector.hpp"
#include "SmallVector.hpp"

namespace xcdat {

/*
 * BASE/CHECK representation using pointer-based byte-oriented DACs.
 * */
class FastDacBc {
public:
  static constexpr uint32_t kFirstBits = 7;

  FastDacBc() {}
  FastDacBc(const std::vector<BcElement>& bc);

  ~FastDacBc() {}

  uint32_t base(uint32_t i) const {
    return access_(i * 2) ^ i;
  }
  uint32_t link(uint32_t i) const {
    return std::get<0>(values_)[i * 2] | (links_[leaves_.rank(i)] << 8);
  }
  uint32_t check(uint32_t i) const {
    return access_(i * 2 + 1) ^ i;
  }

  bool is_leaf(uint32_t i) const {
    return leaves_[i];
  }
  bool is_used(uint32_t i) const {
    return check(i) != i;
  }

  size_t num_nodes() const {
    return std::get<0>(values_).size() / 2;
  }
  size_t num_used_nodes() const {
    return num_nodes() - num_free_nodes_;
  }
  size_t num_free_nodes() const {
    return num_free_nodes_;
  }

  size_t size_in_bytes() const;

  void show_stat(std::ostream& os) const;

  void write(std::ostream& os) const;
  void read(std::istream& is);

  void swap(FastDacBc& rhs);

  FastDacBc(const FastDacBc&) = delete;
  FastDacBc& operator=(const FastDacBc&) = delete;

private:
  std::tuple<
    std::vector<uint8_t>,
    std::vector<uint16_t>,
    std::vector<uint32_t>
  > values_;
  std::array<std::vector<uint32_t>, 2> ranks_;
  BitVector leaves_;
  SmallVector links_;
  size_t num_free_nodes_ = 0;

  uint32_t access_(uint32_t i) const;
};

} //namespace - xcdat

#endif //XCDAT_FAST_DAC_BC_HPP_
