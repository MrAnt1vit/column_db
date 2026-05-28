#pragma once
#include "operator.hpp"
#include "../core/types.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace columnar {

class GlobalCountOperator : public IOperator {
public:
    explicit GlobalCountOperator(std::unique_ptr<IOperator> child);
    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> m_child;
    bool m_done = false;
};

struct AggSpec {
    enum class Kind { Sum, Count, Avg };
    Kind kind;
    size_t batchColIdx = 0;
    DataType srcType = DataType::Int64;
};

class GlobalAggregateOperator : public IOperator {
public:
    GlobalAggregateOperator(std::unique_ptr<IOperator> child,
                            std::vector<AggSpec> specs);

    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> m_child;
    std::vector<AggSpec> m_specs;
    bool m_done = false;

    struct State {
        i128 accum = 0;
        int64_t  count = 0;
    };
    std::vector<State> m_states;
};

} // namespace columnar