#pragma once
#include "../columnar/reader.hpp"
#include "operator.hpp"

#include <memory>
#include <vector>

namespace columnar {

class ScanOperator : public IOperator {
public:
  ScanOperator(std::shared_ptr<ColumnarReader> reader,
               std::vector<size_t> columnIndices);

  std::optional<Batch> Next() override;

private:
  std::shared_ptr<ColumnarReader> m_reader;
  std::vector<size_t> m_columnIndices;
  size_t m_nextBlock = 0;
};

} // namespace columnar