#include "csv_compare.hpp"
#include "../csv/parser.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace columnar {

struct LogicalCsvReader::Impl {
  std::ifstream file;
  char delimiter;
  std::string buffer;
  bool incomplete;

  Impl(const std::filesystem::path &path, char delim)
      : file(path), delimiter(delim), incomplete(false) {}
};

LogicalCsvReader::LogicalCsvReader(const std::filesystem::path &path,
                                   char delimiter)
    : pImpl(std::make_unique<Impl>(path, delimiter)) {}

LogicalCsvReader::~LogicalCsvReader() = default;

void LogicalCsvReader::reset() {
  pImpl->file.clear();
  pImpl->file.seekg(0);
  pImpl->buffer.clear();
  pImpl->incomplete = false;
}

std::vector<std::string> LogicalCsvReader::nextRow() {
  std::string line;
  while (std::getline(pImpl->file, line)) {
    if (pImpl->incomplete) {
      pImpl->buffer += '\n' + line;
    } else {
      pImpl->buffer = std::move(line);
    }

    bool inside = false;
    for (size_t i = 0; i < pImpl->buffer.size(); ++i) {
      if (pImpl->buffer[i] == '"') {
        if (i + 1 < pImpl->buffer.size() && pImpl->buffer[i + 1] == '"') {
          ++i;
        } else {
          inside = !inside;
        }
      }
    }
    if (!inside) {
      auto row = Parser::parseLine(pImpl->buffer, pImpl->delimiter);
      pImpl->buffer.clear();
      pImpl->incomplete = false;
      return row;
    } else {
      pImpl->incomplete = true;
    }
  }
  if (pImpl->incomplete && !pImpl->buffer.empty()) {
    auto row = Parser::parseLine(pImpl->buffer, pImpl->delimiter);
    pImpl->buffer.clear();
    pImpl->incomplete = false;
    return row;
  }
  return {};
}

bool compareCsvFilesLogical(const std::filesystem::path &original,
                            const std::filesystem::path &restored,
                            char delimiter, CsvDiff *diff, bool showProgress) {
  LogicalCsvReader reader1(original, delimiter);
  LogicalCsvReader reader2(restored, delimiter);
  size_t rowNum = 0;
  auto startTime = std::chrono::steady_clock::now();
  auto lastProgress = startTime;

  while (true) {
    auto row1 = reader1.nextRow();
    auto row2 = reader2.nextRow();
    if (row1.empty() && row2.empty())
      break;
    ++rowNum;

    if (showProgress) {
      auto now = std::chrono::steady_clock::now();
      if (now - lastProgress >= std::chrono::seconds(1)) {
        double elapsed = std::chrono::duration<double>(now - startTime).count();
        double rowsPerSec = rowNum / elapsed;
        std::cerr << "\r\x1b[K"
                  << "Validating... processed " << rowNum << " logical rows, "
                  << std::fixed << std::setprecision(0) << rowsPerSec
                  << " rows/s" << std::flush;
        lastProgress = now;
      }
    }

    if (row1.empty() != row2.empty()) {
      if (diff)
        *diff = {rowNum, 0, row1.empty() ? "<EOF>" : row1[0],
                 row2.empty() ? "<EOF>" : row2[0]};
      if (showProgress)
        std::cerr << "\n";
      return false;
    }
    size_t maxCols = std::max(row1.size(), row2.size());
    for (size_t col = 0; col < maxCols; ++col) {
      std::string v1 = (col < row1.size()) ? row1[col] : "";
      std::string v2 = (col < row2.size()) ? row2[col] : "";
      if (v1 != v2) {
        if (diff)
          *diff = {rowNum, col, v1, v2};
        if (showProgress)
          std::cerr << "\n";
        return false;
      }
    }
  }

  if (showProgress) {
    auto endTime = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(endTime - startTime).count();
    std::cerr << "\r\x1b[K"
              << "Validation finished. Compared " << rowNum
              << " logical rows in " << std::fixed << std::setprecision(2)
              << elapsed << " seconds.\n";
  }
  return true;
}

} // namespace columnar