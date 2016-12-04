#include "FastDacBc.hpp"

namespace xcdat {

FastDacBc::FastDacBc(const std::vector<BcElement>& bc) {
  if (bc.empty()) {
    return;
  }

  BitVectorBuilder leaves(bc.size());
  std::vector<uint32_t> links;

  std::get<0>(values_).reserve(bc.size() * 2);
  ranks_[0].reserve((bc.size() * 2) / 128);

  auto append = [&](uint32_t value, bool is_leaf) {
    if ((std::get<0>(values_).size() % 128) == 0) {
      ranks_[0].push_back(static_cast<uint32_t>(std::get<1>(values_).size()));
    }

    if (is_leaf) {
      std::get<0>(values_).push_back(static_cast<uint8_t>(value & 0xFF));
      links.push_back(value >> 8);
      return;
    }

    if ((value / 128) == 0) {
      std::get<0>(values_).push_back(static_cast<uint8_t>(0 | (value << 1)));
      return;
    } else {
      auto pos = std::get<1>(values_).size() - ranks_[0].back();
      std::get<0>(values_).push_back(static_cast<uint8_t>(1 | (pos << 1)));
    }

    if ((std::get<1>(values_).size() % 32768) == 0) {
      ranks_[1].push_back(static_cast<uint32_t>(std::get<2>(values_).size()));
    }

    if ((value / 32768) == 0) {
      std::get<1>(values_).push_back(static_cast<uint16_t>(0 | (value << 1)));
      return;
    } else {
      auto pos = std::get<2>(values_).size() - ranks_[1].back();
      std::get<1>(values_).push_back(static_cast<uint16_t>(1 | (pos << 1)));
    }

    std::get<2>(values_).push_back(value);
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
  std::get<0>(values_).shrink_to_fit();
  std::get<1>(values_).shrink_to_fit();
  std::get<2>(values_).shrink_to_fit();
  ranks_[0].shrink_to_fit();
  ranks_[1].shrink_to_fit();
  BitVector{leaves}.swap(leaves_);
  SmallVector{links}.swap(links_);
}

size_t FastDacBc::size_in_bytes() const {
  size_t ret = 0;
  ret += size_vector(std::get<0>(values_));
  ret += size_vector(std::get<1>(values_));
  ret += size_vector(std::get<2>(values_));
  for (auto& ranks : ranks_) {
    ret += size_vector(ranks);
  }
  ret += leaves_.size_in_bytes();
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
  show_size_ratio("\tvalues_[0]:", size_vector(std::get<0>(values_)), total_size, os);
  show_size_ratio("\tvalues_[1]:", size_vector(std::get<1>(values_)), total_size, os);
  show_size_ratio("\tvalues_[2]:", size_vector(std::get<2>(values_)), total_size, os);
  show_size_ratio("\tranks_[0]: ", size_vector(ranks_[0]), total_size, os);
  show_size_ratio("\tranks_[1]: ", size_vector(ranks_[1]), total_size, os);
  show_size_ratio("\tleaves_:   ", leaves_.size_in_bytes(), total_size, os);
  show_size_ratio("\tlinks_:    ", links_.size_in_bytes(), total_size, os);
}

void FastDacBc::write(std::ostream& os) const {
  write_vector(std::get<0>(values_), os);
  write_vector(std::get<1>(values_), os);
  write_vector(std::get<2>(values_), os);
  for (auto& ranks : ranks_) {
    write_vector(ranks, os);
  }
  leaves_.write(os);
  links_.write(os);
  write_value(num_free_nodes_, os);
}

void FastDacBc::read(std::istream& is) {
  read_vector(std::get<0>(values_), is);
  read_vector(std::get<1>(values_), is);
  read_vector(std::get<2>(values_), is);
  for (auto& ranks : ranks_) {
    read_vector(ranks, is);
  }
  leaves_.read(is);
  links_.read(is);
  read_value(num_free_nodes_, is);
}

void FastDacBc::swap(FastDacBc& rhs) {
  std::get<0>(values_).swap(std::get<0>(rhs.values_));
  std::get<1>(values_).swap(std::get<1>(rhs.values_));
  std::get<2>(values_).swap(std::get<2>(rhs.values_));
  for (uint32_t i = 0; i < ranks_.size(); ++i) {
    ranks_[i].swap(rhs.ranks_[i]);
  }
  leaves_.swap(rhs.leaves_);
  links_.swap(rhs.links_);
  std::swap(num_free_nodes_, rhs.num_free_nodes_);
}

uint32_t FastDacBc::access_(uint32_t i) const {
  uint32_t value = std::get<0>(values_)[i] >> 1;
  if ((std::get<0>(values_)[i] & 1U) == 0) {
    return value;
  }
  i = ranks_[0][i / 128] + value;
  value = std::get<1>(values_)[i] >> 1;
  if ((std::get<1>(values_)[i] & 1U) == 0) {
    return value;
  }
  i = ranks_[1][i / 32768] + value;
  return std::get<2>(values_)[i];
}

} //namespace - xcdat
