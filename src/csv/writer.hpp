#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace columnar {

class Writer {
public:
  explicit Writer(const std::filesystem::path &path);
  void writeRow(const std::vector<std::string> &row);

private:
  std::ofstream m_out;
  std::string escape(const std::string &val);
};

} // namespace columnar