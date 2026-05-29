#pragma once
#include <filesystem>
#include <string>

namespace columnar {
std::string read_file(const std::filesystem::path &path);
} // namespace columnar