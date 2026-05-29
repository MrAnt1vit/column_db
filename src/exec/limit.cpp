#include "limit.hpp"

#include <algorithm>
#include <vector>

namespace columnar {

LimitOperator::LimitOperator(std::unique_ptr<IOperator> child, size_t limit)
    : m_child(std::move(child)), m_limit(limit) {}

std::optional<Batch> LimitOperator::Next() {
    if (m_emitted >= m_limit) return std::nullopt;
    auto batch = m_child->Next();
    if (!batch) return std::nullopt;
    if (batch->rowCount == 0) return batch;

    size_t remaining = m_limit - m_emitted;
    if (batch->rowCount <= remaining) {
        m_emitted += batch->rowCount;
        return batch;
    }

    std::vector<uint8_t> mask(batch->rowCount, 0);
    std::fill(mask.begin(), mask.begin() + remaining, 1);

    Batch out;
    out.rowCount = remaining;
    out.columns.reserve(batch->columns.size());
    for (const auto& c : batch->columns) {
        out.columns.push_back(c->cloneSelected(mask, remaining));
    }
    m_emitted += remaining;
    return out;
}

} // namespace columnar