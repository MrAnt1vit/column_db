#pragma once
#include "operator.hpp"

#include <cstddef>
#include <memory>

namespace columnar {

class LimitOperator : public IOperator {
public:
    LimitOperator(std::unique_ptr<IOperator> child, size_t limit);
    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> m_child;
    size_t m_limit;
    size_t m_emitted = 0;
};

} // namespace columnar