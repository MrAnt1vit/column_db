#include "reader.hpp"
#include <iostream>
#include <stdexcept>

namespace columnar {

ColumnarReader::ColumnarReader(const std::filesystem::path& path) 
    : m_path(path) {}

std::shared_ptr<RowGroup> ColumnarReader::read() {
    std::ifstream in(m_path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open columnar file for reading");
    }

    in.seekg(-8, std::ios::end);
    uint64_t metaStart;
    in.read(reinterpret_cast<char*>(&metaStart), sizeof(metaStart));
    in.seekg(metaStart, std::ios::beg);

    // Количество строк
    uint64_t rowCount;
    in.read(reinterpret_cast<char*>(&rowCount), sizeof(rowCount));

    // Колонки
    uint32_t colCount;
    in.read(reinterpret_cast<char*>(&colCount), sizeof(colCount));
    
    std::vector<uint64_t> offsets(colCount);
    in.read(reinterpret_cast<char*>(offsets.data()), colCount * sizeof(uint64_t));

    // Схема
    Schema schema;
    for (uint32_t i = 0; i < colCount; ++i) {
        uint32_t nameLen;
        in.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        
        std::string name(nameLen, '\0');
        in.read(name.data(), nameLen);
        int typeCode;
        in.read(reinterpret_cast<char*>(&typeCode), sizeof(typeCode));
        DataType type = (typeCode == 0) ? DataType::INT64 : DataType::STRING;

        schema.addColumn(name, type);
    }

    auto rowGroup = std::make_shared<RowGroup>(schema);
    rowGroup->rowCount = rowCount;

    for (size_t i = 0; i < colCount; ++i) {
        in.seekg(offsets[i], std::ios::beg);
        auto& colPtr = rowGroup->columns[i];
        DataType type = colPtr->getType();

        if (type == DataType::INT64) {
            auto intCol = std::static_pointer_cast<Int64Column>(colPtr);
            intCol->data.resize(rowCount);
            in.read(reinterpret_cast<char*>(intCol->data.data()), rowCount * sizeof(int64_t));
        } 
        else if (type == DataType::STRING) {
            auto strCol = std::static_pointer_cast<StringColumn>(colPtr);
            strCol->data.reserve(rowCount);
            
            for (size_t r = 0; r < rowCount; ++r) {
                uint32_t len;
                in.read(reinterpret_cast<char*>(&len), sizeof(len));
                
                std::string str(len, '\0');
                in.read(str.data(), len);
                strCol->add(std::move(str));
            }
        }
    }

    return rowGroup;
}

} // namespace columnar