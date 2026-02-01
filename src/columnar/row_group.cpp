#include "row_group.hpp"
#include "../core/schema.hpp"

namespace columnar {

RowGroup::RowGroup(const Schema& schema) {
    for (const auto& colDef : schema.getColumns()) {
        if (colDef.type == DataType::INT64) {
            columns.push_back(std::make_shared<Int64Column>());
        } else if (colDef.type == DataType::STRING) {
            columns.push_back(std::make_shared<StringColumn>());
        }
    }
}

} // namespace columnar