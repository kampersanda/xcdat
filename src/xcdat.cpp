#include <chrono>
#include <iostream>
#include <random>

#include "xcdat.hpp"

using namespace xcdat;

namespace {

constexpr uint32_t kRuns = 10;

using Key = TrieBuilder::Key;

class StopWatch {
public:
  using hrc = std::chrono::high_resolution_clock;

  StopWatch() : tp_{hrc::now()} {}

  double sec() const {
    const auto tp = hrc::now() - tp_;
    return std::chrono::duration<double>(tp).count();
  }
  double milli_sec() const {
    const auto tp = hrc::now() - tp_;
    return std::chrono::duration<double, std::milli>(tp).count();
  }
  double micro_sec() const {
    const auto tp = hrc::now() - tp_;
    return std::chrono::duration<double, std::micro>(tp).count();
  }

private:
  hrc::time_point tp_;
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

void extract_pairs(const std::vector<std::string>& keys, std::vector<Key>& pairs) {
  pairs.clear();
  pairs.resize(keys.size());
  for (size_t i = 0; i < keys.size(); ++i) {
    pairs[i] = {reinterpret_cast<const uint8_t*>(keys[i].c_str()), keys[i].length()};
  }
};

void show_usage(std::ostream& os) {
  os << "xcdat build <type> <key> <dict>\n";
  os << "\t<type>\t'1' for DACs; '2' for FDACs.\n";
  os << "\t<key> \tinput file of a set of keys.\n";
  os << "\t<dict>\toutput file of the dictionary.\n";
  os << "xcdat query <type> <dict> <limit>\n";
  os << "\t<type> \t'1' for DACs; '2' for FDACs.\n";
  os << "\t<dict> \tinput file of the dictionary.\n";
  os << "\t<limit>\tlimit at lookup (default=10).\n";
  os << "xcdat bench <type> <dict> <key>\n";
  os << "\t<type>\t'1' for DACs; '2' for FDACs.\n";
  os << "\t<dict>\tinput file of the dictionary.\n";
  os << "\t<key> \tinput file of keys for benchmark.\n";
  os.flush();
}

template<bool Fast>
int build(std::vector<std::string>& args) {
  if (args.size() != 4) {
    show_usage(std::cerr);
    return 1;
  }

  std::vector<std::string> strs;
  auto raw_size = read_keys(args[2].c_str(), strs);

  if (raw_size == 0) {
    std::cerr << "open error : " << args[2] << std::endl;
    return 1;
  }

  std::vector<Key> keys;
  extract_pairs(strs, keys);

  Trie<Fast> trie;
  try {
    StopWatch sw;
    trie = TrieBuilder::build<Fast>(keys);
    std::cout << "constr. time: " << sw.sec() << " sec" << std::endl;
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
    trie = Trie<Fast>(ifs);
  }

  size_t limit = 10;
  if (args.size() == 4) {
    limit = std::stoull(args.back());
  }

  std::string query;
//  std::vector<id_type> ids;
//  std::vector<uint8_t> buf;

  while (true){
    putchar('>');
    getline(std::cin, query);
    if (query.empty()){
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
    {
      size_t N = 0;
      auto it = trie.make_prefix_iterator(key, length);
      while (N < limit && it.next()) {
        std::cout << it.id() << '\t';
        std::cout.write(reinterpret_cast<const char*>(it.key().first), it.key().second);
        std::cout << std::endl;
        ++N;
      }

      size_t M = 0;
      while (it.next()) {
        ++M;
      }

      if (M != 0) {
        std::cout << "and more..." << std::endl;
      }
      std::cout << N + M << " found" << std::endl;
    }

    std::cout << "predictive_lookup()" << std::endl;
    {
      size_t N = 0;
      auto it = trie.make_predictive_iterator(key, length);
      while (N < limit && it.next()) {
        std::cout << it.id() << '\t';
        std::cout.write(reinterpret_cast<const char*>(it.key().first), it.key().second);
        std::cout << std::endl;
        ++N;
      }

      size_t M = 0;
      while (it.next()) {
        ++M;
      }

      if (M != 0) {
        std::cout << "and more..." << std::endl;
      }
      std::cout << N + M << " found" << std::endl;
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
    trie = Trie<Fast>(ifs);
  }

  std::vector<std::string> strs;
  if (read_keys(args[3].c_str(), strs) == 0) {
    std::cerr << "open error : " << args[3] << std::endl;
    return 1;
  }

  std::vector<Key> keys;
  extract_pairs(strs, keys);

  std::vector<id_type> ids(keys.size());
  for (size_t i = 0; i < keys.size(); ++i) {
    ids[i] = trie.lookup(keys[i].ptr, keys[i].length);
  }

  {
    std::cout << "Lookup benchmark on " << kRuns << " runs" << std::endl;

    StopWatch sw;
    for (uint32_t r = 0; r < kRuns; ++r) {
      for (size_t i = 0; i < keys.size(); ++i) {
        if (trie.lookup(keys[i].ptr, keys[i].length) == kNotFound) {
          std::cerr << "Failed to lookup " << strs[i] << std::endl;
          return 1;
        }
      }
    }

    std::cout << sw.micro_sec() / kRuns / keys.size() << " us per str" << std::endl;
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

    std::cout << sw.micro_sec() / kRuns / ids.size() << " us per ID" << std::endl;
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
    args.emplace_back(std::string{argv[i]});
  }

  bool is_fast;
  if (args[1][0] == '1') {
    is_fast = false;
  } else if (args[1][0] == '2') {
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
