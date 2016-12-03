#ifndef XCDAT_DAC_BC_HPP_
#define XCDAT_DAC_BC_HPP_

#include <array>

#include "BitVector.hpp"
#include "SmallVector.hpp"

namespace xcdat {

// BASE/CHECK arrays using byte-oriented DACs.
class DacBc {
public:
  static constexpr uint32_t kFirstBits = 8;

  DacBc() {}
  DacBc(const std::vector<BcItem>& bc);

  ~DacBc() {}

  uint32_t base(uint32_t i) const {
    return access_(i * 2) ^ i;
  }
  uint32_t link(uint32_t i) const {
    return values_[0][i * 2] | (links_[leaves_.rank(i)] << 8);
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
    return values_[0].size() / 2;
  }
  size_t num_used_nodes() const {
    return num_nodes() - num_free_nodes_;
  }
  size_t num_free_nodes() const {
    return num_free_nodes_;
  }

  size_t size_in_bytes() const;

  void show_stat(std::ostream &os) const;

  void write(std::ostream &os) const;
  void read(std::istream &is);

  void swap(DacBc& rhs);

  DacBc(const DacBc&) = delete;
  DacBc& operator=(const DacBc&) = delete;

private:
  std::array<std::vector<uint8_t>, 4> values_;
  std::array<BitVector, 3> flags_;
  BitVector leaves_;
  SmallVector links_;
  uint8_t max_level_ = 0;
  size_t num_free_nodes_ = 0;

  uint32_t access_(uint32_t i) const;
};

} //namespace - xcdat

#endif //XCDAT_DAC_BC_HPP_
