#include "writer.hpp"
#include <fstream>
#include <iostream>

namespace columnar {

ColumnarWriter::ColumnarWriter(const std::filesystem::path& path, const Schema& schema)
    : m_path(path), m_schema(schema) {}

void ColumnarWriter::write(const RowGroup& rowGroup) {
    std::ofstream out(m_path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot open output file");
    }

    std::vector<uint64_t> columnOffsets;
    for (const auto& col : rowGroup.columns) {
        columnOffsets.push_back(static_cast<uint64_t>(out.tellp()));

        if (col->getType() == DataType::INT64) {
            auto intCol = std::static_pointer_cast<Int64Column>(col);
            out.write(reinterpret_cast<const char*>(intCol->data.data()), 
                      intCol->data.size() * sizeof(int64_t));
        } 
        else if (col->getType() == DataType::STRING) {
            auto strCol = std::static_pointer_cast<StringColumn>(col);
            for (const auto& s : strCol->data) {
                uint32_t len = static_cast<uint32_t>(s.size());
                out.write(reinterpret_cast<const char*>(&len), sizeof(len));
                out.write(s.data(), len);
            }
        }
    }

    uint64_t metaStart = static_cast<uint64_t>(out.tellp());

    // Количество строк
    uint64_t rows = rowGroup.rowCount;
    out.write(reinterpret_cast<const char*>(&rows), sizeof(rows));

    // Сколько колонок
    uint32_t cols = static_cast<uint32_t>(columnOffsets.size());
    out.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
    out.write(reinterpret_cast<const char*>(columnOffsets.data()), 
              columnOffsets.size() * sizeof(uint64_t));

    // Схема
    for (const auto& def : m_schema.getColumns()) {
        uint32_t nameLen = static_cast<uint32_t>(def.name.size());
        out.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        out.write(def.name.data(), nameLen);
        int typeCode = (def.type == DataType::INT64) ? 0 : 1;
        out.write(reinterpret_cast<const char*>(&typeCode), sizeof(typeCode));
    }

    out.write(reinterpret_cast<const char*>(&metaStart), sizeof(metaStart));
}

} // namespace columnar