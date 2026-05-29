#pragma once
#include <filesystem>

namespace columnar {

int runQuery(int queryId, const std::filesystem::path &columnarPath,
             const std::filesystem::path &outputPath);

} // namespace columnar