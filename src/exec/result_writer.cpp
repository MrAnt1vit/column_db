#include "result_writer.hpp"

#include <ostream>

namespace columnar {

void writeBatchToCsv(std::ostream &out, const Batch &batch) {
  for (size_t r = 0; r < batch.rowCount; ++r) {
    for (size_t c = 0; c < batch.columns.size(); ++c) {
      if (c > 0)
        out << ',';
      out << batch.columns[c]->getString(r);
    }
    out << '\n';
  }
}

} // namespace columnar