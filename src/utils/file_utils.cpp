#include "file_utils.hpp"
#include <fstream>
#include <stdexcept>

namespace columnar {

std::string read_file(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (!file) {
    throw std::runtime_error("Cannot open file: " + path.string());
  }

  std::string content;
  file.seekg(0, std::ios::end);
  content.resize(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(content.data(), content.size());
  return content;
}

} // namespace columnar