#pragma once
#include <filesystem>
#include "../core/schema.hpp"
#include "row_group.hpp"

namespace columnar {

class ColumnarWriter {
public:
    ColumnarWriter(const std::filesystem::path& path, const Schema& schema);
    void write(const RowGroup& rowGroup);

private:
    std::filesystem::path m_path;
    Schema m_schema;
};

} // namespace columnar