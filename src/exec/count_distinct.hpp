#pragma once
#include "operator.hpp"
#include "../core/types.hpp"

#include <cstddef>
#include <memory>

namespace columnar {

class CountDistinctOperator : public IOperator {
public:
    CountDistinctOperator(std::unique_ptr<IOperator> child,
                          size_t batchColIdx,
                          DataType srcType);

    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> m_child;
    size_t   m_batchColIdx;
    DataType m_srcType;
    bool     m_done = false;
};

} // namespace columnar