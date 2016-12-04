#include "DacBc.hpp"

namespace xcdat {

DacBc::DacBc(const std::vector<BcElement>& bc) {
  if (bc.empty()) {
    return;
  }

  std::array<BitVectorBuilder, 4> flags;
  BitVectorBuilder leaves(bc.size());
  std::vector<uint32_t> links;

  values_[0].reserve(bc.size() * 2);
  flags[0].reserve(bc.size() * 2);
  links.reserve(bc.size());

  max_level_ = 0;

  auto append = [&](uint32_t value, bool is_leaf) {
    if (is_leaf) {
      links.push_back(value >> 8);
      values_[0].push_back(static_cast<uint8_t>(value & 0xFF));
      flags[0].push_back(false);
      return;
    }

    uint8_t level = 0;
    values_[level].push_back(static_cast<uint8_t>(value & 0xFF));
    flags[level].push_back(true);
    value >>= 8;

    while (value) {
      ++level;
      values_[level].push_back(static_cast<uint8_t>(value & 0xFF));
      flags[level].push_back(true);
      value >>= 8;
    }
    flags[level].set_bit(flags[level].size() - 1, false);
    max_level_ = std::max(max_level_, level);
  };

  for (uint32_t i = 0; i < bc.size(); ++i) {
    if (!bc[i].is_used) {
      append(0, false);
      append(0, false);
      ++num_free_nodes_;
    } else {
      const auto is_leaf = bc[i].is_leaf;
      leaves.set_bit(i, is_leaf);
      !is_leaf ? append(bc[i].base ^ i, false) : append(bc[i].base, true);
      append(bc[i].check ^ i, false);
    }
  }

  // release
  for (uint8_t i = 0; i < max_level_; ++i) {
    values_[i].shrink_to_fit();
    BitVector{flags[i]}.swap(flags_[i]);
  }
  values_[max_level_].shrink_to_fit();
  BitVector{leaves}.swap(leaves_);
  SmallVector{links}.swap(links_);
}

size_t DacBc::size_in_bytes() const {
  size_t ret = 0;
  for (auto& values : values_) {
    ret += size_vector(values);
  }
  for (auto& flags : flags_) {
    ret += flags.size_in_bytes();
  }
  ret += leaves_.size_in_bytes();
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
  show_size_ratio("\tvalues_[0]:", size_vector(values_[0]), total_size, os);
  show_size_ratio("\tvalues_[1]:", size_vector(values_[1]), total_size, os);
  show_size_ratio("\tvalues_[2]:", size_vector(values_[2]), total_size, os);
  show_size_ratio("\tvalues_[3]:", size_vector(values_[3]), total_size, os);
  show_size_ratio("\tflags_[0]: ", flags_[0].size_in_bytes(), total_size, os);
  show_size_ratio("\tflags_[1]: ", flags_[1].size_in_bytes(), total_size, os);
  show_size_ratio("\tflags_[2]: ", flags_[2].size_in_bytes(), total_size, os);
  show_size_ratio("\tleaves_:   ", leaves_.size_in_bytes(), total_size, os);
  show_size_ratio("\tlinks_:    ", links_.size_in_bytes(), total_size, os);
}

void DacBc::write(std::ostream& os) const {
  for (auto& values : values_) {
    write_vector(values, os);
  }
  for (auto& flags : flags_) {
    flags.write(os);
  }
  leaves_.write(os);
  links_.write(os);
  write_value(max_level_, os);
  write_value(num_free_nodes_, os);
}

void DacBc::read(std::istream& is) {
  for (auto& values : values_) {
    read_vector(values, is);
  }
  for (auto& flags : flags_) {
    flags.read(is);
  }
  leaves_.read(is);
  links_.read(is);
  read_value(max_level_, is);
  read_value(num_free_nodes_, is);
}

void DacBc::swap(DacBc& rhs) {
  for (uint32_t i = 0; i < values_.size(); ++i) {
    values_[i].swap(rhs.values_[i]);
  }
  for (uint32_t i = 0; i < flags_.size(); ++i) {
    flags_[i].swap(rhs.flags_[i]);
  }
  leaves_.swap(rhs.leaves_);
  links_.swap(rhs.links_);
  std::swap(max_level_, rhs.max_level_);
  std::swap(num_free_nodes_, rhs.num_free_nodes_);
}

uint32_t DacBc::access_(uint32_t i) const {
  uint8_t level = 0;
  uint32_t value = values_[level][i];
  while (level < max_level_) {
    if (!flags_[level][i]) {
      break;
    }
    i = flags_[level].rank(i);
    ++level;
    value |= static_cast<uint32_t>(values_[level][i]) << (level * 8);
  }
  return value;
}

} //namespace - xcdat
