#pragma once
#include <filesystem>

namespace columnar {

class Converter {
public:
  // CSV -> Columnar
  static void csvToColumnar(const std::filesystem::path &csvPath,
                            const std::filesystem::path &schemaPath,
                            const std::filesystem::path &outputPath,
                            char delimiter = ',');

  // Columnar -> CSV
  static void columnarToCsv(const std::filesystem::path &columnarPath,
                            const std::filesystem::path &csvOutputPath,
                            char delimiter = ',');
};

} // namespace columnar