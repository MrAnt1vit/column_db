#pragma once
#include "../columnar/row_group.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace columnar {

struct Batch {
  size_t rowCount = 0;
  std::vector<std::shared_ptr<ColumnData>> columns;
};

} // namespace columnar