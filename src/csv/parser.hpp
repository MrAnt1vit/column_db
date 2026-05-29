#pragma once
#include <string>
#include <vector>

namespace columnar {

class Parser {
public:
  static std::vector<std::string> parseLine(const std::string &line,
                                            char delimiter);
  static void resetBuffer();
};

} // namespace columnar