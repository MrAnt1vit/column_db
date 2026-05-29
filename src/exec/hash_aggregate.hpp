#pragma once

#include "operator.hpp"
#include "aggregate.hpp"
#include "../core/types.hpp"
#include "../columnar/row_group.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace columnar {

struct KeySpec {
    size_t   batchColIdx = 0;
    DataType type        = DataType::Int64;
};

class IGroupAggregateState {
public:
    virtual ~IGroupAggregateState() = default;
    virtual void resize(size_t numGroups) = 0;
    virtual void processBatch(const Batch& batch,
                              size_t srcColIdx,
                              const std::vector<size_t>& groupIdx) = 0;
    virtual std::shared_ptr<ColumnData> emit(size_t numGroups) const = 0;
};

class IKeyAccumulator {
public:
    virtual ~IKeyAccumulator() = default;
    virtual void encodeKeyBytes(const ColumnData& srcCol,
                                size_t i,
                                std::string& out) const = 0;
    virtual void appendFrom(const ColumnData& srcCol, size_t i) = 0;
    virtual std::shared_ptr<ColumnData> emit() = 0;
};

class HashGroupByAggregateOperator : public IOperator {
public:
    HashGroupByAggregateOperator(std::unique_ptr<IOperator> child,
                                 std::vector<KeySpec> keys,
                                 std::vector<AggSpec> specs);
    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> m_child;
    std::vector<KeySpec>       m_keys;
    std::vector<AggSpec>       m_specs;
    bool                       m_done = false;
};

} // namespace columnar