#include "hash_aggregate.hpp"

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace columnar {

class SumState : public IGroupAggregateState {
public:
  explicit SumState(DataType srcType) : m_srcType(srcType) {}

  void resize(size_t n) override { m_accum.resize(n, 0); }

  void processBatch(const Batch &b, size_t srcCol,
                    const std::vector<size_t> &gidx) override {
    switch (m_srcType) {
    case DataType::Int8:
      return processTyped<Int8Column>(b, srcCol, gidx);
    case DataType::Int16:
      return processTyped<Int16Column>(b, srcCol, gidx);
    case DataType::Int32:
      return processTyped<Int32Column>(b, srcCol, gidx);
    case DataType::Int64:
      return processTyped<Int64Column>(b, srcCol, gidx);
    default:
      throw std::runtime_error("Sum: unsupported source type");
    }
  }

  std::shared_ptr<ColumnData> emit(size_t n) const override {
    auto c = std::make_shared<Int128Column>();
    c->data.assign(m_accum.begin(), m_accum.begin() + n);
    return c;
  }

private:
  DataType m_srcType;
  std::vector<i128> m_accum;

  template <typename ColumnT>
  void processTyped(const Batch &b, size_t srcCol,
                    const std::vector<size_t> &gidx) {
    const auto *col = static_cast<const ColumnT *>(b.columns[srcCol].get());
    const auto *data = col->data.data();
    for (size_t i = 0; i < b.rowCount; ++i) {
      m_accum[gidx[i]] += static_cast<i128>(data[i]);
    }
  }
};

class CountState : public IGroupAggregateState {
public:
  void resize(size_t n) override { m_counts.resize(n, 0); }

  void processBatch(const Batch &b, size_t /*srcCol*/,
                    const std::vector<size_t> &gidx) override {
    for (size_t i = 0; i < b.rowCount; ++i)
      ++m_counts[gidx[i]];
  }

  std::shared_ptr<ColumnData> emit(size_t n) const override {
    auto c = std::make_shared<Int64Column>();
    c->data.assign(m_counts.begin(), m_counts.begin() + n);
    return c;
  }

private:
  std::vector<int64_t> m_counts;
};

class AvgState : public IGroupAggregateState {
public:
  explicit AvgState(DataType srcType) : m_srcType(srcType) {}

  void resize(size_t n) override {
    m_accum.resize(n, 0);
    m_counts.resize(n, 0);
  }

  void processBatch(const Batch &b, size_t srcCol,
                    const std::vector<size_t> &gidx) override {
    switch (m_srcType) {
    case DataType::Int8:
      return processTyped<Int8Column>(b, srcCol, gidx);
    case DataType::Int16:
      return processTyped<Int16Column>(b, srcCol, gidx);
    case DataType::Int32:
      return processTyped<Int32Column>(b, srcCol, gidx);
    case DataType::Int64:
      return processTyped<Int64Column>(b, srcCol, gidx);
    default:
      throw std::runtime_error("Avg: unsupported source type");
    }
  }

  std::shared_ptr<ColumnData> emit(size_t n) const override {
    auto c = std::make_shared<Int64Column>();
    c->data.resize(n);
    for (size_t i = 0; i < n; ++i) {
      i128 v = (m_counts[i] == 0)
                   ? i128{0}
                   : (m_accum[i] / static_cast<i128>(m_counts[i]));
      c->data[i] = static_cast<int64_t>(v);
    }
    return c;
  }

private:
  DataType m_srcType;
  std::vector<i128> m_accum;
  std::vector<int64_t> m_counts;

  template <typename ColumnT>
  void processTyped(const Batch &b, size_t srcCol,
                    const std::vector<size_t> &gidx) {
    const auto *col = static_cast<const ColumnT *>(b.columns[srcCol].get());
    const auto *data = col->data.data();
    for (size_t i = 0; i < b.rowCount; ++i) {
      m_accum[gidx[i]] += static_cast<i128>(data[i]);
      m_counts[gidx[i]] += 1;
    }
  }
};

template <typename DistColT>
class CountDistinctState : public IGroupAggregateState {
public:
  using D = typename DistColT::value_type;

  void resize(size_t n) override { m_sets.resize(n); }

  void processBatch(const Batch &b, size_t srcCol,
                    const std::vector<size_t> &gidx) override {
    const auto *col = static_cast<const DistColT *>(b.columns[srcCol].get());
    for (size_t i = 0; i < b.rowCount; ++i) {
      m_sets[gidx[i]].insert(col->data[i]);
    }
  }

  std::shared_ptr<ColumnData> emit(size_t n) const override {
    auto c = std::make_shared<Int64Column>();
    c->data.reserve(n);
    for (size_t i = 0; i < n; ++i) {
      c->data.push_back(static_cast<int64_t>(m_sets[i].size()));
    }
    return c;
  }

private:
  std::vector<std::unordered_set<D>> m_sets;
};

static std::unique_ptr<IGroupAggregateState>
makeAggregateState(const AggSpec &spec) {
  switch (spec.kind) {
  case AggSpec::Kind::Sum:
    return std::make_unique<SumState>(spec.srcType);
  case AggSpec::Kind::Count:
    return std::make_unique<CountState>();
  case AggSpec::Kind::Avg:
    return std::make_unique<AvgState>(spec.srcType);
  case AggSpec::Kind::CountDistinct:
    switch (spec.srcType) {
    case DataType::Int8:
      return std::make_unique<CountDistinctState<Int8Column>>();
    case DataType::Int16:
      return std::make_unique<CountDistinctState<Int16Column>>();
    case DataType::Int32:
      return std::make_unique<CountDistinctState<Int32Column>>();
    case DataType::Int64:
      return std::make_unique<CountDistinctState<Int64Column>>();
    case DataType::Date:
      return std::make_unique<CountDistinctState<DateColumn>>();
    case DataType::DateTime:
      return std::make_unique<CountDistinctState<DateTimeColumn>>();
    case DataType::Char:
      return std::make_unique<CountDistinctState<CharColumn>>();
    case DataType::String:
      return std::make_unique<CountDistinctState<StringColumn>>();
    default:
      throw std::runtime_error("CountDistinct: unsupported source type");
    }
  case AggSpec::Kind::Min:
  case AggSpec::Kind::Max:
    throw std::runtime_error(
        "Min/Max are not yet supported in HashGroupByAggregateOperator");
  }
  throw std::runtime_error("Unknown AggSpec::Kind");
}

template <typename KeyColT>
static std::pair<std::shared_ptr<ColumnData>, size_t> runGroupByAggregate(
    IOperator &child, size_t keyIdx, const std::vector<AggSpec> &specs,
    std::vector<std::unique_ptr<IGroupAggregateState>> &states) {
  using K = typename KeyColT::value_type;

  std::unordered_map<K, size_t> keyToGroup;
  auto keyCol = std::make_shared<KeyColT>();
  std::vector<size_t> groupIdx;

  while (auto batch = child.Next()) {
    if (batch->rowCount == 0)
      continue;
    if (keyIdx >= batch->columns.size())
      throw std::runtime_error("HashGroupByAggregate: key column not in batch");

    const auto *kcol =
        static_cast<const KeyColT *>(batch->columns[keyIdx].get());

    groupIdx.resize(batch->rowCount);
    for (size_t i = 0; i < batch->rowCount; ++i) {
      K k = kcol->data[i];
      auto [it, inserted] = keyToGroup.try_emplace(k, keyToGroup.size());
      if (inserted)
        keyCol->data.push_back(k);
      groupIdx[i] = it->second;
    }

    const size_t numGroups = keyCol->data.size();
    for (auto &s : states)
      s->resize(numGroups);
    for (size_t si = 0; si < specs.size(); ++si) {
      states[si]->processBatch(*batch, specs[si].batchColIdx, groupIdx);
    }
  }

  return {std::move(keyCol), keyToGroup.size()};
}

static std::pair<std::shared_ptr<ColumnData>, size_t> dispatchGroupByAggregate(
    IOperator &child, size_t keyIdx, DataType keyType,
    const std::vector<AggSpec> &specs,
    std::vector<std::unique_ptr<IGroupAggregateState>> &states) {
  switch (keyType) {
  case DataType::Int8:
    return runGroupByAggregate<Int8Column>(child, keyIdx, specs, states);
  case DataType::Int16:
    return runGroupByAggregate<Int16Column>(child, keyIdx, specs, states);
  case DataType::Int32:
    return runGroupByAggregate<Int32Column>(child, keyIdx, specs, states);
  case DataType::Int64:
    return runGroupByAggregate<Int64Column>(child, keyIdx, specs, states);
  case DataType::Date:
    return runGroupByAggregate<DateColumn>(child, keyIdx, specs, states);
  case DataType::DateTime:
    return runGroupByAggregate<DateTimeColumn>(child, keyIdx, specs, states);
  case DataType::Char:
    return runGroupByAggregate<CharColumn>(child, keyIdx, specs, states);
  case DataType::String:
    return runGroupByAggregate<StringColumn>(child, keyIdx, specs, states);
  default:
    throw std::runtime_error("HashGroupByAggregate: unsupported key type");
  }
}

HashGroupByAggregateOperator::HashGroupByAggregateOperator(
    std::unique_ptr<IOperator> child, size_t keyBatchColIdx, DataType keyType,
    std::vector<AggSpec> specs)
    : m_child(std::move(child)), m_keyIdx(keyBatchColIdx), m_keyType(keyType),
      m_specs(std::move(specs)) {}

std::optional<Batch> HashGroupByAggregateOperator::Next() {
  if (m_done)
    return std::nullopt;
  m_done = true;

  std::vector<std::unique_ptr<IGroupAggregateState>> states;
  states.reserve(m_specs.size());
  for (const auto &spec : m_specs)
    states.push_back(makeAggregateState(spec));

  auto [keyCol, numGroups] =
      dispatchGroupByAggregate(*m_child, m_keyIdx, m_keyType, m_specs, states);

  Batch out;
  out.rowCount = numGroups;
  out.columns.reserve(1 + states.size());
  out.columns.push_back(std::move(keyCol));
  for (auto &s : states)
    out.columns.push_back(s->emit(numGroups));
  return out;
}

} // namespace columnar