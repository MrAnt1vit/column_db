#pragma once
#include "../core/schema.hpp"
#include "parser.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace columnar {

class Reader {
public:
  static Schema readSchema(const std::filesystem::path &path);

  template <typename F>
  static void readRows(const std::filesystem::path &path, char delimiter,
                       F &&processRow, size_t expectedColumns = 0) {
    std::ifstream file(path);
    if (!file.is_open()) {
      throw std::runtime_error("Cannot open file: " + path.string());
    }
    std::string buffer;
    bool incomplete = false;
    std::string line;
    while (std::getline(file, line)) {
      if (incomplete) {
        buffer += '\n' + line;
      } else {
        buffer = std::move(line);
      }

      int balance = 0;
      for (size_t i = 0; i < buffer.size(); ++i) {
        if (buffer[i] == '"') {
          if (i + 1 < buffer.size() && buffer[i + 1] == '"') {
            ++i;
          } else {
            balance ^= 1;
          }
        }
      }
      if (balance == 0) {
        auto row = Parser::parseLine(buffer, delimiter);
        if (expectedColumns > 0) {
          if (row.size() < expectedColumns)
            row.resize(expectedColumns);
          else if (row.size() > expectedColumns)
            row.resize(expectedColumns);
        }
        processRow(row);
        buffer.clear();
        incomplete = false;
      } else {
        incomplete = true;
      }
    }
    if (incomplete && !buffer.empty()) {
      auto row = Parser::parseLine(buffer, delimiter);
      if (expectedColumns > 0) {
        if (row.size() < expectedColumns)
          row.resize(expectedColumns);
        else if (row.size() > expectedColumns)
          row.resize(expectedColumns);
      }
      processRow(row);
    }
  }
};

} // namespace columnar