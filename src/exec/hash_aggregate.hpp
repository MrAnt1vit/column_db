#pragma once

#include "../columnar/row_group.hpp"
#include "../core/types.hpp"
#include "aggregate.hpp"
#include "operator.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace columnar {

class IGroupAggregateState {
public:
  virtual ~IGroupAggregateState() = default;

  virtual void resize(size_t numGroups) = 0;

  virtual void processBatch(const Batch &batch, size_t srcColIdx,
                            const std::vector<size_t> &groupIdx) = 0;

  virtual std::shared_ptr<ColumnData> emit(size_t numGroups) const = 0;
};

class HashGroupByAggregateOperator : public IOperator {
public:
  HashGroupByAggregateOperator(std::unique_ptr<IOperator> child,
                               size_t keyBatchColIdx, DataType keyType,
                               std::vector<AggSpec> specs);

  std::optional<Batch> Next() override;

private:
  std::unique_ptr<IOperator> m_child;
  size_t m_keyIdx;
  DataType m_keyType;
  std::vector<AggSpec> m_specs;
  bool m_done = false;
};

} // namespace columnar