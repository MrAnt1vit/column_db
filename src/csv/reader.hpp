#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include "../core/schema.hpp"

namespace columnar {

class Reader {
public:
    static Schema readSchema(const std::filesystem::path& path);
    static std::vector<std::vector<std::string>> readRows(const std::filesystem::path& path, char delimiter = ',');
};

} // namespace columnar