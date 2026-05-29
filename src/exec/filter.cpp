#include "filter.hpp"

#include <stdexcept>

namespace columnar {

FilterOperator::FilterOperator(std::unique_ptr<IOperator> child,
                               std::unique_ptr<IFilterExpression> expr)
    : m_child(std::move(child)), m_expr(std::move(expr)) {}

std::optional<Batch> FilterOperator::Next() {
  while (auto batch = m_child->Next()) {
    if (batch->rowCount == 0)
      continue;
    if (batch->columns.empty()) {
      throw std::runtime_error(
          "FilterOperator received a count-only batch — plan is wrong");
    }

    m_expr->evaluate(*batch, m_mask);

    size_t kept = 0;
    for (uint8_t v : m_mask)
      kept += v;
    if (kept == 0)
      continue;

    Batch result;
    result.rowCount = kept;
    result.columns.reserve(batch->columns.size());
    for (const auto &srcCol : batch->columns) {
      result.columns.push_back(srcCol->cloneSelected(m_mask, kept));
    }
    return result;
  }
  return std::nullopt;
}

} // namespace columnar