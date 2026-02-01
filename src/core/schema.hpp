#pragma once
#include <vector>
#include <string>
#include "types.hpp"

namespace columnar {

struct Column {
    std::string name;
    DataType type;
};

class Schema {
public:
    void addColumn(std::string name, DataType type) {
        columns.push_back({std::move(name), type});
    }

    const std::vector<Column>& getColumns() const {
        return columns;
    }

    size_t size() const {
        return columns.size();
    }

private:
    std::vector<Column> columns;
};

} // namespace columnar