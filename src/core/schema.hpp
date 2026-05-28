#pragma once
#include <vector>
#include <string>
#include "types.hpp"
#include <optional>
#include <string_view>

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

    std::optional<size_t> indexOf(std::string_view name) const {
        for (size_t i = 0; i < columns.size(); ++i) {
            if (columns[i].name == name) return i;
        }
        return std::nullopt;
    }

private:
    std::vector<Column> columns;
};

} // namespace columnar