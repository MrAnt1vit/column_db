#pragma once
#include <vector>
#include <string>
#include <string_view>

namespace columnar {

class Parser {
public:
    static std::vector<std::string> parseLine(std::string_view line, char delimiter);
};

} // namespace columnar