#pragma once
#include "operator.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace columnar {

class IFilterExpression {
public:
  virtual ~IFilterExpression() = default;
  virtual void evaluate(const Batch &batch, std::vector<uint8_t> &outMask) = 0;
};

template <typename ColumnT> class ColNotEqualConst : public IFilterExpression {
public:
  using value_type = typename ColumnT::value_type;

  ColNotEqualConst(size_t batchColIdx, value_type value)
      : m_colIdx(batchColIdx), m_value(value) {}

  void evaluate(const Batch &batch, std::vector<uint8_t> &outMask) override {
    const auto *col =
        static_cast<const ColumnT *>(batch.columns[m_colIdx].get());
    outMask.assign(batch.rowCount, 0);
    const auto *data = col->data.data();
    const value_type v = m_value;
    for (size_t i = 0; i < batch.rowCount; ++i) {
      outMask[i] = static_cast<uint8_t>(data[i] != v);
    }
  }

private:
  size_t m_colIdx;
  value_type m_value;
};

class FilterOperator : public IOperator {
public:
  FilterOperator(std::unique_ptr<IOperator> child,
                 std::unique_ptr<IFilterExpression> expr);

  std::optional<Batch> Next() override;

private:
  std::unique_ptr<IOperator> m_child;
  std::unique_ptr<IFilterExpression> m_expr;
  std::vector<uint8_t> m_mask;
};

} // namespace columnar