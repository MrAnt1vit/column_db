#include "sort.hpp"
#include "../columnar/row_group.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace columnar {

template <typename ColumnT>
static std::shared_ptr<ColumnData>
permuteTyped(const ColumnData &col, const std::vector<size_t> &perm) {
  const auto *typed = static_cast<const ColumnT *>(&col);
  auto out = std::make_shared<ColumnT>();
  out->data.reserve(perm.size());
  for (size_t i : perm)
    out->data.push_back(typed->data[i]);
  return out;
}

std::shared_ptr<ColumnData> static permuteColumn(
    const ColumnData &col, const std::vector<size_t> &perm) {
  switch (col.getType()) {
  case DataType::Int8:
    return permuteTyped<Int8Column>(col, perm);
  case DataType::Int16:
    return permuteTyped<Int16Column>(col, perm);
  case DataType::Int32:
    return permuteTyped<Int32Column>(col, perm);
  case DataType::Int64:
    return permuteTyped<Int64Column>(col, perm);
  case DataType::Date:
    return permuteTyped<DateColumn>(col, perm);
  case DataType::DateTime:
    return permuteTyped<DateTimeColumn>(col, perm);
  case DataType::Char:
    return permuteTyped<CharColumn>(col, perm);
  case DataType::String:
    return permuteTyped<StringColumn>(col, perm);
  case DataType::Int128:
    return permuteTyped<Int128Column>(col, perm);
  default:
    throw std::runtime_error("Sort: unsupported column type");
  }
}

template <typename ColumnT>
static int compareTyped(const ColumnData &col, size_t i, size_t j) {
  const auto *typed = static_cast<const ColumnT *>(&col);
  const auto &a = typed->data[i];
  const auto &b = typed->data[j];
  if (a < b)
    return -1;
  if (a > b)
    return 1;
  return 0;
}

static int compareAt(const ColumnData &col, size_t i, size_t j) {
  switch (col.getType()) {
  case DataType::Int8:
    return compareTyped<Int8Column>(col, i, j);
  case DataType::Int16:
    return compareTyped<Int16Column>(col, i, j);
  case DataType::Int32:
    return compareTyped<Int32Column>(col, i, j);
  case DataType::Int64:
    return compareTyped<Int64Column>(col, i, j);
  case DataType::Date:
    return compareTyped<DateColumn>(col, i, j);
  case DataType::DateTime:
    return compareTyped<DateTimeColumn>(col, i, j);
  case DataType::Char:
    return compareTyped<CharColumn>(col, i, j);
  case DataType::String:
    return compareTyped<StringColumn>(col, i, j);
  case DataType::Int128:
    return compareTyped<Int128Column>(col, i, j);
  default:
    throw std::runtime_error("Sort: unsupported column type");
  }
}

SortOperator::SortOperator(std::unique_ptr<IOperator> child,
                           std::vector<SortKey> keys)
    : m_child(std::move(child)), m_keys(std::move(keys)) {}

std::optional<Batch> SortOperator::Next() {
  if (m_done)
    return std::nullopt;
  m_done = true;

  auto in = m_child->Next();
  if (!in)
    return std::nullopt;

  // Sanity check: no second non-empty batch (TODO: generalize later).
  while (auto extra = m_child->Next()) {
    if (extra->rowCount > 0) {
      throw std::runtime_error(
          "SortOperator currently supports only single-batch input");
    }
  }

  if (in->rowCount == 0)
    return in;

  const size_t n = in->rowCount;
  std::vector<size_t> perm(n);
  std::iota(perm.begin(), perm.end(), 0);

  const auto &keys = m_keys;
  const auto &cols = in->columns;
  std::sort(perm.begin(), perm.end(), [&](size_t a, size_t b) {
    for (const auto &k : keys) {
      int c = compareAt(*cols[k.batchColIdx], a, b);
      if (c != 0)
        return k.ascending ? (c < 0) : (c > 0);
    }
    return false;
  });

  Batch out;
  out.rowCount = n;
  out.columns.reserve(in->columns.size());
  for (const auto &srcCol : in->columns) {
    out.columns.push_back(permuteColumn(*srcCol, perm));
  }
  return out;
}

} // namespace columnar