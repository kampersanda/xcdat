#include <chrono>
#include <iostream>
#include <random>

#include "Trie.hpp"

using namespace xcdat;

namespace {

constexpr uint32_t kRuns = 10;

enum class Times {
  SEC, MILLI, MICRO
};

class StopWatch {
public:
  StopWatch() : tp_(std::chrono::high_resolution_clock::now()) {}
  ~StopWatch() {}

  double operator()(Times time) const {
    const auto tp = std::chrono::high_resolution_clock::now() - tp_;
    switch (time) {
      case Times::SEC:
        return std::chrono::duration<double>(tp).count();
      case Times::MILLI:
        return std::chrono::duration<double, std::milli>(tp).count();
      case Times::MICRO:
        return std::chrono::duration<double, std::micro>(tp).count();
    }
    return 0.0;
  }

  StopWatch(const StopWatch&) = delete;
  StopWatch& operator=(const StopWatch&) = delete;

private:
  std::chrono::high_resolution_clock::time_point tp_;
};

size_t read_keys(const char* file_name, std::vector<std::string>& keys) {
  std::ifstream ifs(file_name);
  if (!ifs) {
    return 0;
  }

  size_t size = 0;
  std::string line;

  while (std::getline(ifs, line)) {
    if (!line.empty()) {
      keys.emplace_back(line);
      size += line.length() + 1; // with terminator
    }
  }
  return size;
}

void extract_pairs(const std::vector<std::string>& keys,
                   std::vector<std::pair<const uint8_t*, size_t>>& pairs) {
  pairs.clear();
  pairs.resize(keys.size());
  for (size_t i = 0; i < keys.size(); ++i) {
    pairs[i] = {reinterpret_cast<const uint8_t*>(keys[i].c_str()), keys[i].length()};
  }
};

void show_usage(std::ostream& os) {
  os << "xcdat build <type> <key> <dict>" << std::endl;
  os << "\t<type>\t'1' for DACs; '2' for FDACs." << std::endl;
  os << "\t<key> \tinput file of a set of keys." << std::endl;
  os << "\t<dict>\toutput file for storing the dictionary." << std::endl;
  os << "xcdat query <type> <dict> <limit>" << std::endl;
  os << "\t<type> \t'1' for DACs; '2' for FDACs." << std::endl;
  os << "\t<dict> \tinput file of the dictionary." << std::endl;
  os << "\t<limit>\tlimit at lookup (default=10)." << std::endl;
  os << "xcdat bench <type> <dict> <key>" << std::endl;
  os << "\t<type>\t'1' for DACs; '2' for FDACs." << std::endl;
  os << "\t<dict>\tinput file of the dictionary." << std::endl;
  os << "\t<key> \tinput file of keys for benchmark." << std::endl;
}

template<bool Fast>
int build(std::vector<std::string>& args) {
  if (args.size() != 4) {
    show_usage(std::cerr);
    return 1;
  }

  std::vector<std::string> keys;
  auto raw_size = read_keys(args[2].c_str(), keys);

  if (raw_size == 0) {
    std::cerr << "open error : " << args[2] << std::endl;
    return 1;
  }

  std::vector<std::pair<const uint8_t*, size_t>> pairs;
  extract_pairs(keys, pairs);

  Trie<Fast> trie;
  try {
    StopWatch sw;
    Trie<Fast>(pairs).swap(trie);
    std::cout << "constr. time: " << sw(Times::SEC) << " sec" << std::endl;
  } catch (const xcdat::TrieBuilder::Exception& ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  std::cout << "cmpr. ratio: " << (double) trie.size_in_bytes() / raw_size << std::endl;
  trie.show_stat(std::cout);

  {
    std::ofstream ofs(args[3]);
    if (!ofs) {
      std::cerr << "open error : " << args[3] << std::endl;
      return 1;
    }
    trie.write(ofs);
  }

  return 0;
}

template<bool Fast>
int query(std::vector<std::string>& args) {
  if (args.size() != 3 && args.size() != 4) {
    show_usage(std::cerr);
    return 1;
  }

  Trie<Fast> trie;
  {
    std::ifstream ifs(args[2]);
    if (!ifs) {
      std::cerr << "open error : " << args[2] << std::endl;
      return 1;
    }
    trie.read(ifs);
  }

  size_t limit = 10;
  if (args.size() == 4) {
    limit = std::stoull(args.back());
  }

  std::string query;
  std::vector<id_type> ids;
  std::vector<uint8_t> buf;

  while (true){
    putchar('>');
    getline(std::cin, query);
    if (query.size() == 0){
      break;
    }

    auto key = reinterpret_cast<const uint8_t*>(query.c_str());
    auto length = query.size();

    std::cout << "lookup()" << std::endl;
    auto id = trie.lookup(key, length);
    if (id == kNotFound) {
      std::cout << "not found" << std::endl;
    } else {
      std::cout << id << '\t' << query << std::endl;
    }

    std::cout << "common_prefix_lookup()" << std::endl;
    ids.clear();
    trie.common_prefix_lookup(key, length, ids);
    std::cout << ids.size() << " found" << std::endl;
    for (size_t i = 0; i < std::min(ids.size(), limit); ++i) {
      buf.clear();
      trie.access(ids[i], buf);
      std::cout << ids[i] << '\t' << buf.data() << std::endl;
    }

    std::cout << "predictive_lookup()" << std::endl;
    ids.clear();
    trie.predictive_lookup(key, length, ids);
    std::cout << ids.size() << " found" << std::endl;
    for (size_t i = 0; i < std::min(ids.size(), limit); ++i) {
      buf.clear();
      trie.access(ids[i], buf);
      std::cout << ids[i] << '\t' << buf.data() << std::endl;
    }
  }

  return 0;
}

template<bool Fast>
int bench(std::vector<std::string>& args) {
  if (args.size() != 4) {
    show_usage(std::cerr);
    return 1;
  }

  Trie<Fast> trie;
  {
    std::ifstream ifs(args[2]);
    if (!ifs) {
      std::cerr << "open error : " << args[2] << std::endl;
      return 1;
    }
    trie.read(ifs);
  }

  std::vector<std::string> keys;
  if (read_keys(args[3].c_str(), keys) == 0) {
    std::cerr << "open error : " << args[3] << std::endl;
    return 1;
  }

  std::vector<std::pair<const uint8_t*, size_t>> pairs;
  extract_pairs(keys, pairs);

  std::vector<id_type> ids(pairs.size());
  for (size_t i = 0; i < pairs.size(); ++i) {
    ids[i] = trie.lookup(pairs[i].first, pairs[i].second);
  }

  {
    std::cout << "Lookup benchmark on " << kRuns << " runs" << std::endl;

    StopWatch sw;
    for (uint32_t r = 0; r < kRuns; ++r) {
      for (size_t i = 0; i < pairs.size(); ++i) {
        if (trie.lookup(pairs[i].first, pairs[i].second) == kNotFound) {
          std::cerr << "Failed to lookup " << keys[i] << std::endl;
          return 1;
        }
      }
    }

    std::cout << sw(Times::MICRO) / kRuns / pairs.size() << " us per str" << std::endl;
  }

  {
    std::cout << "Access benchmark on " << kRuns << " runs" << std::endl;

    StopWatch sw;
    for (uint32_t r = 0; r < kRuns; ++r) {
      for (size_t i = 0; i < ids.size(); ++i) {
        std::vector<uint8_t> key;
        if (!trie.access(ids[i], key)) {
          std::cerr << "Failed to access " << ids[i] << std::endl;
          return 1;
        }
      }
    }

    std::cout << sw(Times::MICRO) / kRuns / ids.size() << " us per ID" << std::endl;
  }

  return 0;
}

} // namespace

int main(int argc, const char* argv[]) {
  if (argc < 3) {
    show_usage(std::cerr);
    return 1;
  }

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.push_back({argv[i]});
  }

  bool is_fast;
  if (args[1].front() == '1') {
    is_fast = false;
  } else if (args[1].front() == '2') {
    is_fast = true;
  } else {
    show_usage(std::cerr);
    return 1;
  }

  if (args[0] == "build") {
    return is_fast ? build<true>(args) : build<false>(args);
  } else if (args[0] == "query") {
    return is_fast ? query<true>(args) : query<false>(args);
  } else if (args[0] == "bench") {
    return is_fast ? bench<true>(args) : bench<false>(args);
  }

  show_usage(std::cerr);
  return 1;
}
