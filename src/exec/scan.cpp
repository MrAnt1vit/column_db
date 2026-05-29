#include "scan.hpp"

namespace columnar {

ScanOperator::ScanOperator(std::shared_ptr<ColumnarReader> reader,
                           std::vector<size_t> columnIndices)
    : m_reader(std::move(reader)), m_columnIndices(std::move(columnIndices)) {}

std::optional<Batch> ScanOperator::Next() {
  if (m_nextBlock >= m_reader->getBlockCount()) {
    return std::nullopt;
  }

  Batch b;
  b.rowCount = m_reader->getBlockRows(m_nextBlock);

  if (!m_columnIndices.empty()) {
    b.columns.reserve(m_columnIndices.size());
    for (size_t idx : m_columnIndices) {
      b.columns.push_back(m_reader->readColumn(m_nextBlock, idx));
    }
  }

  ++m_nextBlock;
  return b;
}

} // namespace columnar