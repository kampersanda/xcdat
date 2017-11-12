#ifndef XCDAT_DAC_BC_HPP_
#define XCDAT_DAC_BC_HPP_

#include "BitVector.hpp"
#include "FitVector.hpp"

namespace xcdat {

// BASE/CHECK representation using byte-oriented DACs.
class DacBc {
public:
  static constexpr id_type WIDTH_L1 {8};

  DacBc() = default;
  ~DacBc() = default;

  explicit DacBc(std::istream &is);
  explicit DacBc(const std::vector<BcPair>& bc, BitVectorBuilder& leaf_flags);

  id_type base(id_type i) const {
    return access_(i * 2) ^ i;
  }
  id_type link(id_type i) const {
    return values_[0][i * 2] | (links_[leaf_flags_.rank(i)] << 8);
  }
  id_type check(id_type i) const {
    return access_(i * 2 + 1) ^ i;
  }

  bool is_leaf(id_type i) const {
    return leaf_flags_[i];
  }
  bool is_used(id_type i) const {
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

  void swap(DacBc& rhs) {
    std::swap(*this, rhs);
  }

  DacBc(const DacBc&) = delete;
  DacBc& operator=(const DacBc&) = delete;

  DacBc(DacBc&&) noexcept = default;
  DacBc& operator=(DacBc&&) noexcept = default;

private:
  Vector<uint8_t> values_[sizeof(id_type)] {};
  BitVector flags_[sizeof(id_type) - 1] {};
  BitVector leaf_flags_ {};
  FitVector links_ {};
  uint8_t max_level_ {};
  size_t num_free_nodes_ {};

  id_type access_(id_type i) const;
};

} //namespace - xcdat

#endif //XCDAT_DAC_BC_HPP_
