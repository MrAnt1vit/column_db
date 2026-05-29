#include "writer.hpp"
#include <stdexcept>

namespace columnar {

ColumnarWriter::ColumnarWriter(const std::filesystem::path &path,
                               const Schema &schema)
    : m_schema(schema) {
  m_out.open(path, std::ios::binary | std::ios::trunc);
  if (!m_out) {
    throw std::runtime_error("Cannot create output file: " + path.string());
  }

  m_out.write("COLM", 4);
  uint32_t version = 2;
  m_out.write(reinterpret_cast<const char *>(&version), sizeof(version));
  uint64_t placeholder = 0;
  m_out.write(reinterpret_cast<const char *>(&placeholder),
              sizeof(placeholder));
  m_metadataOffsetPos = static_cast<uint64_t>(m_out.tellp());
}

ColumnarWriter::~ColumnarWriter() {
  if (m_out.is_open()) {
    try {
      finalize();
    } catch (...) {
    }
  }
}

void ColumnarWriter::writeBlock(const RowGroup &block) {
  const uint64_t blockStart = static_cast<uint64_t>(m_out.tellp());

  uint64_t rows = block.rowCount();
  m_out.write(reinterpret_cast<const char *>(&rows), sizeof(rows));

  const auto &cols = block.getColumns();
  uint32_t numCols = static_cast<uint32_t>(cols.size());
  m_out.write(reinterpret_cast<const char *>(&numCols), sizeof(numCols));

  const uint64_t offsetTablePos = static_cast<uint64_t>(m_out.tellp());
  const std::vector<char> placeholder(
      static_cast<size_t>(numCols) * 2 * sizeof(uint64_t), 0);
  m_out.write(placeholder.data(),
              static_cast<std::streamsize>(placeholder.size()));

  std::vector<std::pair<uint64_t, uint64_t>> colInfo;
  colInfo.reserve(numCols);
  for (const auto &col : cols) {
    const uint64_t before = static_cast<uint64_t>(m_out.tellp());
    col->serialize(m_out);
    const uint64_t after = static_cast<uint64_t>(m_out.tellp());
    colInfo.emplace_back(before - blockStart, after - before);
  }
  const uint64_t blockEnd = static_cast<uint64_t>(m_out.tellp());

  m_out.seekp(static_cast<std::streamoff>(offsetTablePos), std::ios::beg);
  for (const auto &[off, sz] : colInfo) {
    m_out.write(reinterpret_cast<const char *>(&off), sizeof(off));
    m_out.write(reinterpret_cast<const char *>(&sz), sizeof(sz));
  }

  m_out.seekp(static_cast<std::streamoff>(blockEnd), std::ios::beg);

  m_blocks.emplace_back(blockStart, rows);
}

void ColumnarWriter::finalize() {
  uint64_t metadataStart = m_out.tellp();

  // Пишем схему: количество колонок, для каждой – длина имени, имя, тип
  const auto &cols = m_schema.getColumns();
  uint32_t colCount = static_cast<uint32_t>(cols.size());
  m_out.write(reinterpret_cast<const char *>(&colCount), sizeof(colCount));
  for (const auto &col : cols) {
    uint32_t nameLen = static_cast<uint32_t>(col.name.size());
    m_out.write(reinterpret_cast<const char *>(&nameLen), sizeof(nameLen));
    m_out.write(col.name.data(), nameLen);
    int typeCode = static_cast<int>(col.type);
    m_out.write(reinterpret_cast<const char *>(&typeCode), sizeof(typeCode));
  }

  // Пишем количество блоков и массив (offset, rows)
  uint64_t blockCount = m_blocks.size();
  m_out.write(reinterpret_cast<const char *>(&blockCount), sizeof(blockCount));
  for (const auto &blk : m_blocks) {
    m_out.write(reinterpret_cast<const char *>(&blk.first), sizeof(blk.first));
    m_out.write(reinterpret_cast<const char *>(&blk.second),
                sizeof(blk.second));
  }

  // Пишем смещение начала метаданных в конец файла
  m_out.write(reinterpret_cast<const char *>(&metadataStart),
              sizeof(metadataStart));

  // Возвращаемся в начало и записываем смещение метаданных в заголовок
  m_out.seekp(m_metadataOffsetPos - sizeof(uint64_t));
  m_out.write(reinterpret_cast<const char *>(&metadataStart),
              sizeof(metadataStart));

  m_out.close();
}

} // namespace columnar