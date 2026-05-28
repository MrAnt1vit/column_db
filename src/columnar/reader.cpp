#include "reader.hpp"
#include <fstream>
#include <stdexcept>
#include <cstring>

namespace columnar {

ColumnarReader::ColumnarReader(const std::filesystem::path& path) : m_path(path) {}

ColumnarReader::~ColumnarReader() = default;

void ColumnarReader::loadMetadata() {
    std::ifstream in(m_path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open file: " + m_path.string());

    // Читаем заголовок
    char magic[4];
    in.read(magic, 4);
    if (std::string(magic, 4) != "COLM") throw std::runtime_error("Not a COLM file");
    uint32_t version;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 2) throw std::runtime_error("Unsupported version");

    uint64_t metadataOffset;
    in.read(reinterpret_cast<char*>(&metadataOffset), sizeof(metadataOffset));

    // Переходим к метаданным
    in.seekg(metadataOffset, std::ios::beg);

    // Читаем схему
    uint32_t colCount;
    in.read(reinterpret_cast<char*>(&colCount), sizeof(colCount));
    for (uint32_t i = 0; i < colCount; ++i) {
        uint32_t nameLen;
        in.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        std::string name(nameLen, '\0');
        in.read(name.data(), nameLen);
        int typeCode;
        in.read(reinterpret_cast<char*>(&typeCode), sizeof(typeCode));
        DataType type = static_cast<DataType>(typeCode);
        m_schema.addColumn(name, type);
    }

    // Читаем блоки
    uint64_t blockCount;
    in.read(reinterpret_cast<char*>(&blockCount), sizeof(blockCount));
    m_blocks.resize(blockCount);
    for (size_t i = 0; i < blockCount; ++i) {
        in.read(reinterpret_cast<char*>(&m_blocks[i].first), sizeof(m_blocks[i].first));
        in.read(reinterpret_cast<char*>(&m_blocks[i].second), sizeof(m_blocks[i].second));
    }

    m_metadataLoaded = true;
}

RowGroup ColumnarReader::readBlock(size_t blockIdx) const {
    if (!m_metadataLoaded) throw std::runtime_error("Metadata not loaded");
    if (blockIdx >= m_blocks.size()) throw std::runtime_error("Invalid block index");

    std::ifstream in(m_path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open file");

    const uint64_t blockStart = m_blocks[blockIdx].first;
    in.seekg(static_cast<std::streamoff>(blockStart), std::ios::beg);

    uint64_t rows;
    in.read(reinterpret_cast<char*>(&rows), sizeof(rows));
    uint32_t numCols;
    in.read(reinterpret_cast<char*>(&numCols), sizeof(numCols));

    in.seekg(static_cast<std::streamoff>(numCols) * 2 * sizeof(uint64_t),
             std::ios::cur);
    RowGroup block(m_schema);
    block.deserialize(in, rows);
    return block;
}

void ColumnarReader::forEachRow(std::function<void(const std::vector<std::string>&)> callback) const {
    if (!m_metadataLoaded) {
        const_cast<ColumnarReader*>(this)->loadMetadata();
    }

    for (size_t bi = 0; bi < m_blocks.size(); ++bi) {
        RowGroup block = readBlock(bi);
        const auto& cols = block.getColumns();
        size_t rows = block.rowCount();
        for (size_t r = 0; r < rows; ++r) {
            std::vector<std::string> row;
            row.reserve(cols.size());
            for (const auto& col : cols) {
                row.push_back(col->getString(r));
            }
            callback(row);
        }
    }
}

std::shared_ptr<ColumnData>
ColumnarReader::readColumn(size_t blockIdx, size_t columnIdx) const {
    if (!m_metadataLoaded) throw std::runtime_error("Metadata not loaded");
    if (blockIdx >= m_blocks.size()) throw std::runtime_error("Invalid block index");
    if (columnIdx >= m_schema.size()) throw std::runtime_error("Invalid column index");

    std::ifstream in(m_path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open file");

    const uint64_t blockStart = m_blocks[blockIdx].first;
    const uint64_t rows = m_blocks[blockIdx].second;

    const uint64_t entryPos = blockStart
                              + sizeof(uint64_t)
                              + sizeof(uint32_t)
                              + columnIdx * 2 * sizeof(uint64_t);
    in.seekg(static_cast<std::streamoff>(entryPos), std::ios::beg);

    uint64_t colOffset, colSize;
    in.read(reinterpret_cast<char*>(&colOffset), sizeof(colOffset));
    in.read(reinterpret_cast<char*>(&colSize), sizeof(colSize));
    (void)colSize;
    in.seekg(static_cast<std::streamoff>(blockStart + colOffset), std::ios::beg);

    const DataType type = m_schema.getColumns()[columnIdx].type;
    auto col = createColumn(type);
    col->deserialize(in, rows);
    return col;
}

} // namespace columnar