#pragma once
#include <string>
#include <filesystem>

namespace columnar {
    std::string read_file(const std::filesystem::path& path);
} // namespace columnar