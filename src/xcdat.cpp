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

class StringBuffer {
public:
  StringBuffer() {}
  ~StringBuffer() {}

  bool load(const char* file_path) {
    std::ifstream ifs{file_path};
    if (!ifs) {
      return false;
    }

    std::string line;
    while (std::getline(ifs, line)) {
      offsets_.push_back(buffer_.size());
      for (uint8_t c : line) {
        buffer_.push_back(c);
      }
    }
    offsets_.push_back(buffer_.size());

    buffer_.shrink_to_fit();
    offsets_.shrink_to_fit();

    return true;
  }

  void extract(std::vector<CharRange>& strings) const {
    strings.clear();
    strings.resize(offsets_.size() - 1);
    for (size_t i = 0; i < strings.size(); ++i) {
      strings[i] = {buffer_.data() + offsets_[i], buffer_.data() + offsets_[i + 1]};
    }
  }

  size_t raw_size() const { // including a terminators
    return buffer_.size() + offsets_.size() - 1;
  }

  StringBuffer(const StringBuffer&) = delete;
  StringBuffer& operator=(const StringBuffer&) = delete;

private:
  std::vector<uint8_t> buffer_;
  std::vector<size_t> offsets_;
};

void show_usage(std::ostream& os) {
  os << "xcdat build <type> <str> <dict>" << std::endl;
  os << "\t<type>\t'1' for DACs; '2' for FDACs." << std::endl;
  os << "\t<str> \tinput file of the set of strings." << std::endl;
  os << "\t<dict>\toutput file for storing the dictionary." << std::endl;
  os << "xcdat query <type> <dict> <limit>" << std::endl;
  os << "\t<type> \t'1' for DACs; '2' for FDACs." << std::endl;
  os << "\t<dict> \tinput file of the dictionary." << std::endl;
  os << "\t<limit>\tlimit at lookup (default=10)." << std::endl;
  os << "xcdat bench <type> <dict> <str>" << std::endl;
  os << "\t<type>\t'1' for DACs; '2' for FDACs." << std::endl;
  os << "\t<dict>\tinput file of the dictionary." << std::endl;
  os << "\t<str> \tinput file of strings for benchmark." << std::endl;
}

template<bool Fast>
int build(std::vector<std::string>& args) {
  if (args.size() != 4) {
    show_usage(std::cerr);
    return 1;
  }

  StringBuffer buffer;
  if (!buffer.load(args[2].c_str())) {
    std::cerr << "open error : " << args[2] << std::endl;
    return 1;
  }

  std::vector<CharRange> strings;
  buffer.extract(strings);

  Trie<Fast> trie;
  try {
    StopWatch sw;
    Trie<Fast>{strings}.swap(trie);
    std::cout << "constr. time: " << sw(Times::SEC) << " sec" << std::endl;
  } catch (const xcdat::Exception& ex) {
    std::cerr << ex.what() << " : " << ex.file_name() << " : "
              << ex.line() << " : " << ex.func_name() << std::endl;
    return 1;
  }

  std::cout << "cmpr. ratio: " << (double) trie.size_in_bytes() / buffer.raw_size() << std::endl;
  std::cout << "trie stat:" << std::endl;
  trie.show_stat(std::cout);

  {
    std::ofstream ofs{args[3]};
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
    std::ifstream ifs{args[2]};
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
  std::vector<uint32_t> ids;

  while (true){
    putchar('>');
    getline(std::cin, query);
    if (query.size() == 0){
      break;
    }

    std::cout << "lookup()" << std::endl;
    auto id = trie.lookup({query});
    if (id == kNotFound) {
      std::cout << "not found" << std::endl;
    } else {
      std::cout << id << '\t' << query << std::endl;
    }

    std::cout << "common_prefix_lookup()" << std::endl;
    ids.clear();
    trie.common_prefix_lookup({query}, ids);
    std::cout << ids.size() << " found" << std::endl;
    for (size_t i = 0; i < std::min(ids.size(), limit); ++i) {
      std::cout << ids[i] << '\t' << trie.access(ids[i]) << std::endl;
    }

    std::cout << "predictive_lookup()" << std::endl;
    ids.clear();
    trie.predictive_lookup({query}, ids);
    std::cout << ids.size() << " found" << std::endl;
    for (size_t i = 0; i < std::min(ids.size(), limit); ++i) {
      std::cout << ids[i] << '\t' << trie.access(ids[i]) << std::endl;
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
    std::ifstream ifs{args[2]};
    if (!ifs) {
      std::cerr << "open error : " << args[2] << std::endl;
      return 1;
    }
    trie.read(ifs);
  }

  StringBuffer buffer;
  if (!buffer.load(args[3].c_str())) {
    std::cerr << "open error : " << args[3] << std::endl;
    return 1;
  }

  std::vector<CharRange> strings;
  buffer.extract(strings);

  std::vector<uint32_t> ids(strings.size());
  for (size_t i = 0; i < strings.size(); ++i) {
    ids[i] = trie.lookup(strings[i]);
  }

  {
    std::cout << "Lookup benchmark on " << kRuns << " runs" << std::endl;

    StopWatch sw;
    for (uint32_t r = 0; r < kRuns; ++r) {
      for (size_t i = 0; i < strings.size(); ++i) {
        if (trie.lookup(strings[i]) == kNotFound) {
          std::cerr << "Failed to lookup " << strings[i] << std::endl;
          return 1;
        }
      }
    }

    std::cout << sw(Times::MICRO) / kRuns / strings.size() << " us per str" << std::endl;
  }

  {
    std::cout << "Access benchmark on " << kRuns << " runs" << std::endl;

    StopWatch sw;
    for (uint32_t r = 0; r < kRuns; ++r) {
      for (size_t i = 0; i < ids.size(); ++i) {
        if (trie.access(ids[i]).empty()) {
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
