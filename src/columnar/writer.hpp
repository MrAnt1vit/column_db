#pragma once
#include "../core/schema.hpp"
#include "row_group.hpp"
#include <filesystem>
#include <fstream>
#include <vector>

namespace columnar {

class ColumnarWriter {
public:
  ColumnarWriter(const std::filesystem::path &path, const Schema &schema);
  ~ColumnarWriter();

  void writeBlock(const RowGroup &block);

  void finalize();

private:
  std::ofstream m_out;
  Schema m_schema;
  std::vector<std::pair<uint64_t, uint64_t>> m_blocks; // offset, rows
  uint64_t m_metadataOffsetPos;
};

} // namespace columnar