#include "writer.hpp"
#include <stdexcept>

namespace columnar {

ColumnarWriter::ColumnarWriter(const std::filesystem::path& path, const Schema& schema)
    : m_schema(schema)
{
    m_out.open(path, std::ios::binary | std::ios::trunc);
    if (!m_out) {
        throw std::runtime_error("Cannot create output file: " + path.string());
    }

    m_out.write("COLM", 4);
    uint32_t version = 1;
    m_out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    uint64_t placeholder = 0;
    m_out.write(reinterpret_cast<const char*>(&placeholder), sizeof(placeholder));
    m_metadataOffsetPos = static_cast<uint64_t>(m_out.tellp());
}

ColumnarWriter::~ColumnarWriter() {
    if (m_out.is_open()) {
        try { finalize(); } catch(...) {}
    }
}

void ColumnarWriter::writeBlock(const RowGroup& block) {
    uint64_t offset = m_out.tellp();
    block.serialize(m_out);
    m_blocks.emplace_back(offset, block.rowCount());
}

void ColumnarWriter::finalize() {
    uint64_t metadataStart = m_out.tellp();

    // Пишем схему: количество колонок, для каждой – длина имени, имя, тип
    const auto& cols = m_schema.getColumns();
    uint32_t colCount = static_cast<uint32_t>(cols.size());
    m_out.write(reinterpret_cast<const char*>(&colCount), sizeof(colCount));
    for (const auto& col : cols) {
        uint32_t nameLen = static_cast<uint32_t>(col.name.size());
        m_out.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        m_out.write(col.name.data(), nameLen);
        int typeCode = static_cast<int>(col.type);
        m_out.write(reinterpret_cast<const char*>(&typeCode), sizeof(typeCode));
    }

    // Пишем количество блоков и массив (offset, rows)
    uint64_t blockCount = m_blocks.size();
    m_out.write(reinterpret_cast<const char*>(&blockCount), sizeof(blockCount));
    for (const auto& blk : m_blocks) {
        m_out.write(reinterpret_cast<const char*>(&blk.first), sizeof(blk.first));
        m_out.write(reinterpret_cast<const char*>(&blk.second), sizeof(blk.second));
    }

    // Пишем смещение начала метаданных в конец файла
    m_out.write(reinterpret_cast<const char*>(&metadataStart), sizeof(metadataStart));

    // Возвращаемся в начало и записываем смещение метаданных в заголовок
    m_out.seekp(m_metadataOffsetPos - sizeof(uint64_t));
    m_out.write(reinterpret_cast<const char*>(&metadataStart), sizeof(metadataStart));
    
    m_out.close();
}

} // namespace columnar