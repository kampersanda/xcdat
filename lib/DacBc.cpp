#include <sstream>

#include "xcdat/DacBc.hpp"

namespace xcdat {

DacBc::DacBc(std::istream& is) {
  for (size_t i = 0; i < sizeof(id_type); ++i) {
    values_[i] = Vector<uint8_t>(is);
  }
  for (size_t i = 0; i < sizeof(id_type) - 1; ++i) {
    flags_[i] = BitVector(is);
  }
  leaf_flags_ = BitVector(is);
  links_ = FitVector(is);
  max_level_ = read_value<uint8_t>(is);
  num_free_nodes_ = read_value<size_t>(is);
}

DacBc::DacBc(const std::vector<BcPair>& bc, BitVectorBuilder& leaf_flags) {
  if (bc.empty()) {
    return;
  }

  std::vector<uint8_t> values[sizeof(id_type)];
  BitVectorBuilder flags[sizeof(id_type)];
  std::vector<id_type> links;

  leaf_flags_ = BitVector(leaf_flags, true, false);

  values[0].reserve(bc.size() * 2);
  flags[0].reserve(bc.size() * 2);
  links.reserve(bc.size());

  max_level_ = 0;

  auto append = [&](id_type value) {
    uint8_t level = 0;
    values[level].push_back(static_cast<uint8_t>(value & 0xFF));
    flags[level].push_back(true);
    value >>= 8;
    while (value) {
      ++level;
      values[level].push_back(static_cast<uint8_t>(value & 0xFF));
      flags[level].push_back(true);
      value >>= 8;
    }
    flags[level].set_bit(flags[level].size() - 1, false);
    max_level_ = std::max(max_level_, level);
  };

  auto append_leaf = [&](id_type value) {
    links.push_back(value >> 8);
    values[0].push_back(static_cast<uint8_t>(value & 0xFF));
    flags[0].push_back(false);
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
  for (uint8_t i = 0; i < max_level_; ++i) {
    values_[i] = Vector<uint8_t>(values[i]);
    flags_[i] = BitVector(flags[i], true, false);
  }
  values_[max_level_] = Vector<uint8_t>(values[max_level_]);
  links_ = FitVector(links);
}

size_t DacBc::size_in_bytes() const {
  size_t ret = 0;
  for (auto& values : values_) {
    ret += values.size_in_bytes();
  }
  for (auto& flags : flags_) {
    ret += flags.size_in_bytes();
  }
  ret += leaf_flags_.size_in_bytes();
  ret += links_.size_in_bytes();
  ret += sizeof(max_level_);
  ret += sizeof(num_free_nodes_);
  return ret;
}

void DacBc::show_stat(std::ostream& os) const {
  const auto total_size = size_in_bytes();
  os << "basic statistics of xcdat::DacBc" << std::endl;
  show_size("\tnum links:     ", links_.size(), os);
  show_size("\tbytes per node:", double(total_size) / num_nodes(), os);
  os << "member size statistics of xcdat::DacBc" << std::endl;
  for (int i = 0; i <= max_level_; ++i) {
    std::ostringstream oss;
    oss << "\tvalues_L" << i << ":";
    show_size_ratio(oss.str().c_str(), values_[i].size_in_bytes(), total_size, os);
  }
  for (int i = 0; i < max_level_; ++i) {
    std::ostringstream oss;
    oss << "\tflags_L" << i << ": ";
    show_size_ratio(oss.str().c_str(), flags_[i].size_in_bytes(), total_size, os);
  }
  show_size_ratio("\tleaves:   ", leaf_flags_.size_in_bytes(), total_size, os);
  show_size_ratio("\tlinks:    ", links_.size_in_bytes(), total_size, os);
}

void DacBc::write(std::ostream& os) const {
  for (auto& values : values_) {
    values.write(os);
  }
  for (auto& flags : flags_) {
    flags.write(os);
  }
  leaf_flags_.write(os);
  links_.write(os);
  write_value(max_level_, os);
  write_value(num_free_nodes_, os);
}

id_type DacBc::access_(id_type i) const {
  uint8_t level = 0;
  id_type value = values_[level][i];
  while (level < max_level_) {
    if (!flags_[level][i]) {
      break;
    }
    i = flags_[level].rank(i);
    ++level;
    value |= static_cast<id_type>(values_[level][i]) << (level * 8);
  }
  return value;
}

} //namespace - xcdat
