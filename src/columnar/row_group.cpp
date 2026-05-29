#include "row_group.hpp"
#include <stdexcept>

namespace columnar {

std::shared_ptr<ColumnData> createColumn(DataType type) {
  switch (type) {
  case DataType::Int8:
    return std::make_shared<TypedColumn<int8_t, DataType::Int8>>();
  case DataType::Int16:
    return std::make_shared<TypedColumn<int16_t, DataType::Int16>>();
  case DataType::Int32:
    return std::make_shared<TypedColumn<int32_t, DataType::Int32>>();
  case DataType::Int64:
    return std::make_shared<TypedColumn<int64_t, DataType::Int64>>();
  case DataType::String:
    return std::make_shared<StringColumn>();
  case DataType::Date:
    return std::make_shared<DateColumn>();
  case DataType::DateTime:
    return std::make_shared<DateTimeColumn>();
  case DataType::Char:
    return std::make_shared<CharColumn>();
  default:
    throw std::runtime_error("Unknown DataType");
  }
}

RowGroup::RowGroup(const Schema &schema, size_t maxUncompressedBytes)
    : m_schema(schema), m_maxBytes(maxUncompressedBytes), m_currentBytes(0),
      m_rows(0) {
  for (const auto &colDef : schema.getColumns()) {
    m_columns.push_back(createColumn(colDef.type));
  }
}

void RowGroup::addRow(const std::vector<std::string> &row) {
  if (row.size() != m_columns.size()) {
    throw std::runtime_error("Row size does not match schema");
  }
  for (size_t i = 0; i < row.size(); ++i) {
    std::string value = row[i];
    m_currentBytes += m_columns[i]->addFromString(value);
  }
  ++m_rows;
}

bool RowGroup::isFull() const {
  return (m_maxBytes > 0 && m_currentBytes >= m_maxBytes);
}

void RowGroup::clear() {
  for (auto &col : m_columns) {
    col->clear();
  }
  m_currentBytes = 0;
  m_rows = 0;
}

void RowGroup::serialize(std::ostream &out) const {
  for (const auto &col : m_columns) {
    col->serialize(out);
  }
}

void RowGroup::deserialize(std::istream &in, uint64_t rows) {
  m_rows = rows;
  for (auto &col : m_columns) {
    col->deserialize(in, m_rows);
  }
  m_currentBytes = 0;
}

} // namespace columnar