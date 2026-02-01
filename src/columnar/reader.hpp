#pragma once
#include <filesystem>
#include <fstream>
#include <memory>
#include "row_group.hpp"
#include "../core/schema.hpp"

namespace columnar {

class ColumnarReader {
public:
    explicit ColumnarReader(const std::filesystem::path& path);
    std::shared_ptr<RowGroup> read();

private:
    std::filesystem::path m_path;
    Schema readSchema(std::ifstream& in);
};

} // namespace columnar