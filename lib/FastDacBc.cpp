#include "xcdat/FastDacBc.hpp"

namespace xcdat {

FastDacBc::FastDacBc(std::istream& is) {
  values_L1_ = Vector<uint8_t>(is);
  values_L2_ = Vector<uint16_t>(is);
  values_L3_ = Vector<uint32_t>(is);
#ifdef XCDAT_X64
  values_L4_ = Vector<uint64_t>(is);
#endif
  for (size_t i = 0; i < LAYERS - 1; ++i) {
    ranks_[i] = Vector<id_type>(is);
  }
  leaf_flags_ = BitVector(is);
  links_ = FitVector(is);
  num_free_nodes_ = read_value<size_t>(is);
}

FastDacBc::FastDacBc(const std::vector<BcPair>& bc,
                     BitVectorBuilder& leaf_flags) {
  if (bc.empty()) {
    return;
  }

  std::vector<uint8_t> values_L1;
  std::vector<uint16_t> values_L2;
  std::vector<uint32_t> values_L3;
#ifdef XCDAT_X64
  std::vector<uint64_t> values_L4;
#endif
  std::vector<id_type> ranks[LAYERS - 1];
  std::vector<id_type> links;
  leaf_flags_ = BitVector(leaf_flags, true, false);

  ranks[0].reserve((bc.size() * 2) / 128);

  auto append = [&](id_type value) {
    if ((values_L1.size() % BLOCK_SIZE_L1) == 0) {
      ranks[0].push_back(static_cast<id_type>(values_L2.size()));
    }
    if ((value / BLOCK_SIZE_L1) == 0) {
      values_L1.push_back(static_cast<uint8_t>(0 | (value << 1)));
      return;
    } else {
      auto pos = values_L2.size() - ranks[0].back();
      values_L1.push_back(static_cast<uint8_t>(1 | (pos << 1)));
    }

    if ((values_L2.size() % BLOCK_SIZE_L2) == 0) {
      ranks[1].push_back(static_cast<id_type>(values_L3.size()));
    }
    if ((value / BLOCK_SIZE_L2) == 0) {
      values_L2.push_back(static_cast<uint16_t>(0 | (value << 1)));
      return;
    } else {
      auto pos = values_L3.size() - ranks[1].back();
      values_L2.push_back(static_cast<uint16_t>(1 | (pos << 1)));
    }

#ifdef XCDAT_X64
    if ((values_L3.size() % BLOCK_SIZE_L3) == 0) {
      ranks[1].push_back(static_cast<id_type>(values_L4.size()));
    }
    if ((value / BLOCK_SIZE_L3) == 0) {
      values_L3.push_back(static_cast<uint32_t>(0 | (value << 1)));
      return;
    } else {
      auto pos = values_L4.size() - ranks[1].back();
      values_L3.push_back(static_cast<uint32_t>(1 | (pos << 1)));
    }
    values_L4.push_back(value);
#else
    values_L3.push_back(value);
#endif
  };

  auto append_leaf = [&](id_type value) {
    if ((values_L1.size() % BLOCK_SIZE_L1) == 0) {
      ranks[0].push_back(static_cast<id_type>(values_L2.size()));
    }
    values_L1.push_back(static_cast<uint8_t>(value & 0xFF));
    links.push_back(value >> 8);
  };


  for (id_type i = 0; i < bc.size(); ++i) {
    if (leaf_flags_[i]) {
      append_leaf(bc[i].base);
    } else {
      append(bc[i].base ^ i);
    }
    append(bc[i].check ^ i);
    if (bc[i].check == i) {
      ++num_free_nodes_;
    }
  }

  // release
  values_L1_ = Vector<uint8_t>(values_L1);
  values_L2_ = Vector<uint16_t>(values_L2);
  values_L3_ = Vector<uint32_t>(values_L3);
#ifdef XCDAT_X64
  values_L4_ = Vector<uint64_t>(values_L4);
#endif
  for (uint8_t j = 0; j < LAYERS - 1; ++j) {
    ranks_[j] = Vector<id_type>(ranks[j]);
  }
  links_ = FitVector(links);
}

size_t FastDacBc::size_in_bytes() const {
  size_t ret = 0;
  ret += values_L1_.size_in_bytes();
  ret += values_L2_.size_in_bytes();
  ret += values_L3_.size_in_bytes();
#ifdef XCDAT_X64
  ret += values_L4_.size_in_bytes();
#endif
  for (auto& ranks : ranks_) {
    ret += ranks.size_in_bytes();
  }
  ret += leaf_flags_.size_in_bytes();
  ret += links_.size_in_bytes();
  ret += sizeof(num_free_nodes_);
  return ret;
}

void FastDacBc::show_stat(std::ostream& os) const {
  const auto total_size = size_in_bytes();
  os << "basic statistics of xcdat::FastDacBc" << std::endl;
  show_size("\tnum links:     ", links_.size(), os);
  show_size("\tbytes per node:", double(total_size) / num_nodes(), os);
  os << "member size statistics of xcdat::FastDacBc" << std::endl;
  show_size_ratio("\tvalues_L1:", values_L1_.size_in_bytes(), total_size, os);
  show_size_ratio("\tvalues_L2:", values_L2_.size_in_bytes(), total_size, os);
  show_size_ratio("\tvalues_L3:", values_L3_.size_in_bytes(), total_size, os);
#ifdef XCDAT_X64
  show_size_ratio("\tvalues_L4:", values_L4_.size_in_bytes(), total_size, os);
#endif
  show_size_ratio("\tranks_L1: ", ranks_[0].size_in_bytes(), total_size, os);
  show_size_ratio("\tranks_L2: ", ranks_[1].size_in_bytes(), total_size, os);
#ifdef XCDAT_X64
  show_size_ratio("\tranks_L3: ", ranks_[2].size_in_bytes(), total_size, os);
#endif
  show_size_ratio("\tleaves:   ", leaf_flags_.size_in_bytes(), total_size, os);
  show_size_ratio("\tlinks:    ", links_.size_in_bytes(), total_size, os);
}

void FastDacBc::write(std::ostream& os) const {
  values_L1_.write(os);
  values_L2_.write(os);
  values_L3_.write(os);
#ifdef XCDAT_X64
  values_L4_.write(os);
#endif
  for (auto& ranks : ranks_) {
    ranks.write(os);
  }
  leaf_flags_.write(os);
  links_.write(os);
  write_value(num_free_nodes_, os);
}

id_type FastDacBc::access_(id_type i) const {
  uint32_t value = values_L1_[i] >> 1;
  if ((values_L1_[i] & 1U) == 0) {
    return value;
  }
  i = ranks_[0][i / BLOCK_SIZE_L1] + value;
  value = values_L2_[i] >> 1;
  if ((values_L2_[i] & 1U) == 0) {
    return value;
  }
  i = ranks_[1][i / BLOCK_SIZE_L2] + value;
#ifdef XCDAT_X64
  value = values_L3_[i] >> 1;
  if ((values_L3_[i] & 1U) == 0) {
    return value;
  }
  i = ranks_[2][i / BLOCK_SIZE_L3] + value;
  return values_L4_[i];
#else
  return values_L3_[i];
#endif
}

} //namespace - xcdat
