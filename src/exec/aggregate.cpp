#include "aggregate.hpp"
#include "../columnar/row_group.hpp"

#include <stdexcept>

namespace columnar {

GlobalCountOperator::GlobalCountOperator(std::unique_ptr<IOperator> child)
    : m_child(std::move(child)) {}

std::optional<Batch> GlobalCountOperator::Next() {
  if (m_done)
    return std::nullopt;
  m_done = true;

  int64_t count = 0;
  while (auto batch = m_child->Next()) {
    count += static_cast<int64_t>(batch->rowCount);
  }

  auto col = std::make_shared<Int64Column>();
  col->data.push_back(count);

  Batch out;
  out.rowCount = 1;
  out.columns.push_back(std::move(col));
  return out;
}

struct State {
  i128 accum = 0;
  int64_t count = 0;
  i128 extremum = 0;
  bool hasValue = false;
};

template <typename ColumnT>
static void sumColumnAsInt128(const Batch &b, size_t colIdx, i128 &accum,
                              int64_t &count) {
  const auto *col = static_cast<const ColumnT *>(b.columns[colIdx].get());
  const auto *data = col->data.data();
  for (size_t i = 0; i < b.rowCount; ++i) {
    accum += static_cast<i128>(data[i]);
  }
  count += static_cast<int64_t>(b.rowCount);
}

static void sumByType(const Batch &b, size_t colIdx, DataType srcType,
                      i128 &accum, int64_t &count) {
  switch (srcType) {
  case DataType::Int8:
    return sumColumnAsInt128<Int8Column>(b, colIdx, accum, count);
  case DataType::Int16:
    return sumColumnAsInt128<Int16Column>(b, colIdx, accum, count);
  case DataType::Int32:
    return sumColumnAsInt128<Int32Column>(b, colIdx, accum, count);
  case DataType::Int64:
    return sumColumnAsInt128<Int64Column>(b, colIdx, accum, count);
  default:
    throw std::runtime_error("Sum/Avg: unsupported source type");
  }
}

template <typename ColumnT>
static void minMaxColumnAsInt128(const Batch &b, size_t colIdx, bool isMin,
                                 i128 &extremum, bool &hasValue) {
  const auto *col = static_cast<const ColumnT *>(b.columns[colIdx].get());
  const auto *data = col->data.data();
  for (size_t i = 0; i < b.rowCount; ++i) {
    i128 v = static_cast<i128>(data[i]);
    if (!hasValue) {
      extremum = v;
      hasValue = true;
      continue;
    }
    if (isMin) {
      if (v < extremum)
        extremum = v;
    } else {
      if (v > extremum)
        extremum = v;
    }
  }
}

static void minMaxByType(const Batch &b, size_t colIdx, DataType srcType,
                         bool isMin, i128 &extremum, bool &hasValue) {
  switch (srcType) {
  case DataType::Int8:
    return minMaxColumnAsInt128<Int8Column>(b, colIdx, isMin, extremum,
                                            hasValue);
  case DataType::Int16:
    return minMaxColumnAsInt128<Int16Column>(b, colIdx, isMin, extremum,
                                             hasValue);
  case DataType::Int32:
    return minMaxColumnAsInt128<Int32Column>(b, colIdx, isMin, extremum,
                                             hasValue);
  case DataType::Int64:
    return minMaxColumnAsInt128<Int64Column>(b, colIdx, isMin, extremum,
                                             hasValue);
  case DataType::Date:
    return minMaxColumnAsInt128<DateColumn>(b, colIdx, isMin, extremum,
                                            hasValue);
  case DataType::DateTime:
    return minMaxColumnAsInt128<DateTimeColumn>(b, colIdx, isMin, extremum,
                                                hasValue);
  default:
    throw std::runtime_error("Min/Max: unsupported source type");
  }
}

static std::shared_ptr<ColumnData> makeScalarColumn(DataType srcType,
                                                    i128 value) {
  switch (srcType) {
  case DataType::Int8: {
    auto c = std::make_shared<Int8Column>();
    c->data.push_back(static_cast<int8_t>(value));
    return c;
  }
  case DataType::Int16: {
    auto c = std::make_shared<Int16Column>();
    c->data.push_back(static_cast<int16_t>(value));
    return c;
  }
  case DataType::Int32: {
    auto c = std::make_shared<Int32Column>();
    c->data.push_back(static_cast<int32_t>(value));
    return c;
  }
  case DataType::Int64: {
    auto c = std::make_shared<Int64Column>();
    c->data.push_back(static_cast<int64_t>(value));
    return c;
  }
  case DataType::Date: {
    auto c = std::make_shared<DateColumn>();
    c->data.push_back(static_cast<int32_t>(value));
    return c;
  }
  case DataType::DateTime: {
    auto c = std::make_shared<DateTimeColumn>();
    c->data.push_back(static_cast<int64_t>(value));
    return c;
  }
  default:
    throw std::runtime_error("makeScalarColumn: unsupported type");
  }
}

GlobalAggregateOperator::GlobalAggregateOperator(
    std::unique_ptr<IOperator> child, std::vector<AggSpec> specs)
    : m_child(std::move(child)), m_specs(std::move(specs)) {}

std::optional<Batch> GlobalAggregateOperator::Next() {
  if (m_done)
    return std::nullopt;
  m_done = true;

  std::vector<State> states(m_specs.size());

  while (auto batch = m_child->Next()) {
    if (batch->rowCount == 0)
      continue;
    for (size_t si = 0; si < m_specs.size(); ++si) {
      const auto &spec = m_specs[si];
      auto &st = states[si];
      switch (spec.kind) {
      case AggSpec::Kind::Sum:
      case AggSpec::Kind::Avg:
        sumByType(*batch, spec.batchColIdx, spec.srcType, st.accum, st.count);
        break;
      case AggSpec::Kind::Count:
        st.count += static_cast<int64_t>(batch->rowCount);
        break;
      case AggSpec::Kind::Min:
        minMaxByType(*batch, spec.batchColIdx, spec.srcType, true, st.extremum,
                     st.hasValue);
        break;
      case AggSpec::Kind::Max:
        minMaxByType(*batch, spec.batchColIdx, spec.srcType, false, st.extremum,
                     st.hasValue);
        break;
      case AggSpec::Kind::CountDistinct:
        throw std::runtime_error(
            "GlobalAggregateOperator: CountDistinct is supported only "
            "in HashGroupByAggregateOperator / CountDistinctOperator");
      }
    }
  }

  Batch out;
  out.rowCount = 1;
  out.columns.reserve(m_specs.size());
  for (size_t si = 0; si < m_specs.size(); ++si) {
    const auto &spec = m_specs[si];
    const auto &st = states[si];
    switch (spec.kind) {
    case AggSpec::Kind::Sum: {
      auto c = std::make_shared<Int128Column>();
      c->data.push_back(st.accum);
      out.columns.push_back(std::move(c));
      break;
    }
    case AggSpec::Kind::Count: {
      auto c = std::make_shared<Int64Column>();
      c->data.push_back(st.count);
      out.columns.push_back(std::move(c));
      break;
    }
    case AggSpec::Kind::Avg: {
      auto c = std::make_shared<Int64Column>();
      i128 v =
          (st.count == 0) ? i128{0} : (st.accum / static_cast<i128>(st.count));
      c->data.push_back(static_cast<int64_t>(v));
      out.columns.push_back(std::move(c));
      break;
    }
    case AggSpec::Kind::Min:
    case AggSpec::Kind::Max:
      out.columns.push_back(makeScalarColumn(spec.srcType, st.extremum));
      break;
    case AggSpec::Kind::CountDistinct:
      throw std::runtime_error(
          "GlobalAggregateOperator: CountDistinct emit not supported");
    }
  }
  return out;
}

} // namespace columnar