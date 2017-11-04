#ifndef XCDAT_FAST_DAC_BC_HPP_
#define XCDAT_FAST_DAC_BC_HPP_

#include <tuple>

#include "BitVector.hpp"
#include "FitVector.hpp"
#include "Vector.hpp"

namespace xcdat {

// BASE/CHECK representation using pointer-based byte-oriented DACs.
class FastDacBc {
public:
  static constexpr id_type kWidthL1 = 7;
#ifdef XCDAT_X64
  static constexpr uint8_t kLayers = 4;
#else
  static constexpr uint8_t kLayers = 3;
#endif

  static constexpr id_type kBlockLenL1 = 1U << 7;
  static constexpr id_type kBlockLenL2 = 1U << 15;
#ifdef XCDAT_X64
  static constexpr id_type kBlockLenL3 = 1U << 31;
#endif

  FastDacBc() = default;
  explicit FastDacBc(std::istream& is);
  explicit FastDacBc(const std::vector<BcPair>& bc,
                     BitVectorBuilder& leaf_flags);

  ~FastDacBc() = default;

  id_type base(id_type i) const {
    return access_(i * 2) ^ i;
  }
  id_type link(id_type i) const {
    return values_L1_[i * 2] | (links_[leaf_flags_.rank(i)] << 8);
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
    return values_L1_.size() / 2;
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

  FastDacBc(const FastDacBc&) = delete;
  FastDacBc& operator=(const FastDacBc&) = delete;

  FastDacBc(FastDacBc&&) noexcept = default;
  FastDacBc& operator=(FastDacBc&&) noexcept = default;

private:
  Vector<uint8_t> values_L1_ {};
  Vector<uint16_t> values_L2_ {};
  Vector<uint32_t> values_L3_ {};
#ifdef XCDAT_X64
  Vector<uint64_t> values_L4_ {};
#endif
  Vector<id_type> ranks_[kLayers - 1] {};
  BitVector leaf_flags_ {};
  FitVector links_ {};
  size_t num_free_nodes_ {};

  id_type access_(id_type i) const;
};

} //namespace - xcdat

#endif //XCDAT_FAST_DAC_BC_HPP_
