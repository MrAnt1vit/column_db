#include "converter.hpp"

#include "../csv/reader.hpp"
#include "../columnar/writer.hpp"
#include "../columnar/row_group.hpp"
#include "../core/schema.hpp"
#include "../csv/writer.hpp"
#include "../columnar/reader.hpp"
// #include <print> // C++23

namespace columnar {

void Converter::csvToColumnar(const std::filesystem::path& csvPath,
                              const std::filesystem::path& schemaPath,
                              const std::filesystem::path& outputPath) {
    // std::println("Starting conversion: {} -> {}", csvPath.string(), outputPath.string());
    
    auto schema = Reader::readSchema(schemaPath);
    char delimiter = ','; 
    auto rawRows = Reader::readRows(csvPath, delimiter);
    RowGroup rowGroup(schema);
    size_t rowsProcessed = 0;
    for (const auto& row : rawRows) {
        if (row.size() != schema.size()) {
            // битые строки
            continue;
        }

        for (size_t i = 0; i < row.size(); ++i) {
            auto& colPtr = rowGroup.columns[i];
            DataType type = schema.getColumns()[i].type;

            if (type == DataType::INT64) {
                int64_t val = 0;
                try {
                    if (!row[i].empty()) val = std::stoll(row[i]);
                } catch (...) {
                    // пишем 0
                }
                std::static_pointer_cast<Int64Column>(colPtr)->add(val);
            } 
            else if (type == DataType::STRING) {
                std::static_pointer_cast<StringColumn>(colPtr)->add(row[i]);
            }
        }
        rowsProcessed++;
    }
    rowGroup.rowCount = rowsProcessed;
    ColumnarWriter writer(outputPath, schema);
    writer.write(rowGroup);

    // std::println("Conversion complete. Processed {} rows.", rowsProcessed);
}

void Converter::columnarToCsv(const std::filesystem::path& columnarPath,
                              const std::filesystem::path& csvOutputPath) {
    
    // std::println("Starting export: {} -> {}", columnarPath.string(), csvOutputPath.string());

    ColumnarReader reader(columnarPath);
    auto rowGroup = reader.read();
    Writer writer(csvOutputPath);
    for (size_t rowIdx = 0; rowIdx < rowGroup->rowCount; ++rowIdx) {
        std::vector<std::string> rowValues;
        rowValues.reserve(rowGroup->columns.size());

        for (const auto& colPtr : rowGroup->columns) {
            DataType type = colPtr->getType();
            
            if (type == DataType::INT64) {
                auto intCol = std::static_pointer_cast<Int64Column>(colPtr);
                rowValues.push_back(std::to_string(intCol->data[rowIdx]));
            } 
            else if (type == DataType::STRING) {
                auto strCol = std::static_pointer_cast<StringColumn>(colPtr);
                rowValues.push_back(strCol->data[rowIdx]);
            }
        }
        writer.writeRow(rowValues);
    }

    // std::println("Export complete. Wrote {} rows.", rowGroup->rowCount);
}

} // namespace columnar