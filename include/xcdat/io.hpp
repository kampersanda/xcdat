#pragma once

#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace xcdat::io {

static std::vector<std::string> load_strings(std::string_view filepath) {
    std::ifstream ifs(filepath);
    if (!ifs) {
        return {};
    }
    std::vector<std::string> strs;
    for (std::string str; std::getline(ifs, str);) {
        strs.push_back(str);
    }
    return strs;
}

}  // namespace xcdat::io
