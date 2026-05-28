#pragma once
#include "batch.hpp"

#include <iosfwd>

namespace columnar {

void writeBatchToCsv(std::ostream& out, const Batch& batch);

} // namespace columnar