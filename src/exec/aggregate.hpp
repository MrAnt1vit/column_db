#pragma once

#include "operator.hpp"
#include "../core/types.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace columnar {

struct AggSpec {
    enum class Kind {
        Sum,
        Count,
        Avg,
        Min,
        Max,
        CountDistinct
    };
    Kind     kind;
    size_t   batchColIdx = 0;
    DataType srcType     = DataType::Int64;
};

class GlobalCountOperator : public IOperator {
public:
    explicit GlobalCountOperator(std::unique_ptr<IOperator> child);
    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> m_child;
    bool m_done = false;
};

class GlobalAggregateOperator : public IOperator {
public:
    GlobalAggregateOperator(std::unique_ptr<IOperator> child,
                            std::vector<AggSpec> specs);
    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> m_child;
    std::vector<AggSpec>       m_specs;
    bool                       m_done = false;
};

} // namespace columnar