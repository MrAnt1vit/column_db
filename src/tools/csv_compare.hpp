#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <memory>

namespace columnar {

struct CsvDiff {
    size_t line = 0;
    size_t column = 0;
    std::string left;
    std::string right;
};

class LogicalCsvReader {
public:
    explicit LogicalCsvReader(const std::filesystem::path& path, char delimiter = ',');
    ~LogicalCsvReader();

    std::vector<std::string> nextRow();
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

bool compareCsvFilesLogical(const std::filesystem::path& original,
                            const std::filesystem::path& restored,
                            char delimiter,
                            CsvDiff* diff = nullptr,
                            bool showProgress = false);

} // namespace columnar