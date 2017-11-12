#include <chrono>
#include <iostream>
#include <random>

#include "xcdat.hpp"

using namespace xcdat;

namespace {

constexpr int RUNS = 10;

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
  std::ifstream ifs{file_name};
  if (!ifs) {
    return 0;
  }

  size_t size = 0;
  for (std::string line; std::getline(ifs, line);) {
    keys.push_back(line);
    size += line.length() + 1; // with terminator
  }

  return size;
}

std::vector<std::string_view>
extract_views(const std::vector<std::string>& keys) {
  std::vector<std::string_view> views(keys.size());
  for (size_t i = 0; i < keys.size(); ++i) {
    views[i] = keys[i];
  }
  return views;
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

  std::vector<std::string> keys_buffer;
  auto raw_size = read_keys(args[2].c_str(), keys_buffer);

  if (raw_size == 0) {
    std::cerr << "open error : " << args[2] << std::endl;
    return 1;
  }

  auto keys = extract_views(keys_buffer);

  Trie<Fast> trie;
  try {
    StopWatch sw;
    trie = TrieBuilder::build<Fast>(keys);
    std::cout << "constr. time: " << sw.sec() << " sec" << std::endl;
  } catch (const xcdat::TrieBuilder::Exception& ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  std::cout << "cmpr. ratio: "
            << static_cast<double>(trie.size_in_bytes()) / raw_size
            << std::endl;
  trie.show_stat(std::cout);

  {
    std::ofstream ofs{args[3] + (Fast ? ".fdac" : ".dac")};
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

  while (true){
    std::cout << "> " << std::flush;
    std::getline(std::cin, query);
    if (query.empty()){
      break;
    }

    std::cout << "Lookup" << std::endl;
    auto id = trie.lookup(query);
    if (id == Trie<Fast>::NOT_FOUND) {
      std::cout << "not found" << std::endl;
    } else {
      std::cout << id << '\t' << query << std::endl;
    }

    std::cout << "Common Prefix Lookup" << std::endl;
    {
      size_t N = 0;
      auto it = trie.make_prefix_iterator(query);
      while (N < limit && it.next()) {
        std::cout << it.id() << '\t' << it.key() << std::endl;
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

    std::cout << "Predictive Lookup" << std::endl;
    {
      size_t N = 0;
      auto it = trie.make_predictive_iterator(query);
      while (N < limit && it.next()) {
        std::cout << it.id() << '\t' << it.key() << std::endl;
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

  std::vector<std::string> keys_buffer;
  if (read_keys(args[3].c_str(), keys_buffer) == 0) {
    std::cerr << "open error : " << args[3] << std::endl;
    return 1;
  }

  auto keys = extract_views(keys_buffer);

  std::vector<id_type> ids(keys.size());
  for (size_t i = 0; i < keys.size(); ++i) {
    ids[i] = trie.lookup(keys[i]);
  }

  {
    std::cout << "Lookup benchmark on " << RUNS << " runs" << std::endl;

    StopWatch sw;
    for (uint32_t r = 0; r < RUNS; ++r) {
      for (size_t i = 0; i < keys.size(); ++i) {
        if (trie.lookup(keys[i]) == Trie<Fast>::NOT_FOUND) {
          std::cerr << "Failed to lookup " << keys_buffer[i] << std::endl;
          return 1;
        }
      }
    }

    std::cout << sw.micro_sec() / RUNS / keys.size()
              << " us per str" << std::endl;
  }

  {
    std::cout << "Access benchmark on " << RUNS << " runs" << std::endl;

    StopWatch sw;
    for (uint32_t r = 0; r < RUNS; ++r) {
      for (auto id : ids) {
        auto dec = trie.access(id);
        if (dec.empty()) {
          std::cerr << "Failed to access " << id << std::endl;
          return 1;
        }
      }
    }

    std::cout << sw.micro_sec() / RUNS / ids.size()
              << " us per ID" << std::endl;
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
