#pragma once
#include "operator.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace columnar {

struct SortKey {
    size_t batchColIdx;
    bool ascending = true;
};

class SortOperator : public IOperator {
public:
    SortOperator(std::unique_ptr<IOperator> child, std::vector<SortKey> keys);
    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> m_child;
    std::vector<SortKey> m_keys;
    bool m_done = false;
};

} // namespace columnar