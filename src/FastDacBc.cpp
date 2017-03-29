#include "FastDacBc.hpp"

namespace xcdat {

FastDacBc::FastDacBc(const std::vector<BcPair>& bc, BitVectorBuilder& leaf_flags) {
  if (bc.empty()) {
    return;
  }

  std::vector<uint8_t> values_L1;
  std::vector<uint16_t> values_L2;
  std::vector<uint32_t> values_L3;
#ifdef XCDAT64
  std::vector<uint64_t> values_L4;
#endif
  std::array<std::vector<id_type>, kLayers - 1> ranks;

  std::vector<id_type> links;
  BitVector(leaf_flags, true, false).swap(leaf_flags_);

  ranks[0].reserve((bc.size() * 2) / 128);

  auto append = [&](id_type value) {
    if ((values_L1.size() % kBlockLenL1) == 0) {
      ranks[0].push_back(static_cast<id_type>(values_L2.size()));
    }
    if ((value / kBlockLenL1) == 0) {
      values_L1.push_back(static_cast<uint8_t>(0 | (value << 1)));
      return;
    } else {
      auto pos = values_L2.size() - ranks[0].back();
      values_L1.push_back(static_cast<uint8_t>(1 | (pos << 1)));
    }

    if ((values_L2.size() % kBlockLenL2) == 0) {
      ranks[1].push_back(static_cast<id_type>(values_L3.size()));
    }
    if ((value / kBlockLenL2) == 0) {
      values_L2.push_back(static_cast<uint16_t>(0 | (value << 1)));
      return;
    } else {
      auto pos = values_L3.size() - ranks[1].back();
      values_L2.push_back(static_cast<uint16_t>(1 | (pos << 1)));
    }

#ifdef XCDAT64
    if ((values_L3.size() % kBlockLenL3) == 0) {
      ranks[1].push_back(static_cast<id_type>(values_L4.size()));
    }
    if ((value / kBlockLenL3) == 0) {
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
    if ((values_L1.size() % kBlockLenL1) == 0) {
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
  values_L1_.steal(values_L1);
  values_L2_.steal(values_L2);
  values_L3_.steal(values_L3);
#ifdef XCDAT64
  values_L4_.steal(values_L4);
#endif
  for (uint8_t j = 0; j < ranks.size(); ++j) {
    ranks_[j].steal(ranks[j]);
  }
  FitVector(links).swap(links_);
}

size_t FastDacBc::size_in_bytes() const {
  size_t ret = 0;
  ret += values_L1_.size_in_bytes();
  ret += values_L2_.size_in_bytes();
  ret += values_L3_.size_in_bytes();
#ifdef XCDAT64
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
#ifdef XCDAT64
  show_size_ratio("\tvalues_L4:", values_L4_.size_in_bytes(), total_size, os);
#endif
  show_size_ratio("\tranks_L1: ", ranks_[0].size_in_bytes(), total_size, os);
  show_size_ratio("\tranks_L2: ", ranks_[1].size_in_bytes(), total_size, os);
#ifdef XCDAT64
  show_size_ratio("\tranks_L3: ", ranks_[2].size_in_bytes(), total_size, os);
#endif
  show_size_ratio("\tleaves:   ", leaf_flags_.size_in_bytes(), total_size, os);
  show_size_ratio("\tlinks:    ", links_.size_in_bytes(), total_size, os);
}

void FastDacBc::write(std::ostream& os) const {
  values_L1_.write(os);
  values_L2_.write(os);
  values_L3_.write(os);
#ifdef XCDAT64
  values_L4_.write(os);
#endif
  for (auto& ranks : ranks_) {
    ranks.write(os);
  }
  leaf_flags_.write(os);
  links_.write(os);
  write_value(num_free_nodes_, os);
}

void FastDacBc::read(std::istream& is) {
  values_L1_.read(is);
  values_L2_.read(is);
  values_L3_.read(is);
#ifdef XCDAT64
  values_L4_.read(is);
#endif
  for (auto& ranks : ranks_) {
    ranks.read(is);
  }
  leaf_flags_.read(is);
  links_.read(is);
  read_value(num_free_nodes_, is);
}

void FastDacBc::swap(FastDacBc& rhs) {
  values_L1_.swap(rhs.values_L1_);
  values_L2_.swap(rhs.values_L2_);
  values_L3_.swap(rhs.values_L3_);
#ifdef XCDAT64
  values_L4_.swap(rhs.values_L4_);
#endif
  for (uint32_t j = 0; j < ranks_.size(); ++j) {
    ranks_[j].swap(rhs.ranks_[j]);
  }
  leaf_flags_.swap(rhs.leaf_flags_);
  links_.swap(rhs.links_);
  std::swap(num_free_nodes_, rhs.num_free_nodes_);
}

id_type FastDacBc::access_(id_type i) const {
  uint32_t value = values_L1_[i] >> 1;
  if ((values_L1_[i] & 1U) == 0) {
    return value;
  }
  i = ranks_[0][i / kBlockLenL1] + value;
  value = values_L2_[i] >> 1;
  if ((values_L2_[i] & 1U) == 0) {
    return value;
  }
  i = ranks_[1][i / kBlockLenL2] + value;
#ifdef XCDAT64
  value = values_L3_[i] >> 1;
  if ((values_L3_[i] & 1U) == 0) {
    return value;
  }
  i = ranks_[2][i / kBlockLenL3] + value;
  return values_L4_[i];
#else
  return values_L3_[i];
#endif
}

} //namespace - xcdat
