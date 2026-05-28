#pragma once
#include <filesystem>
#include <functional>
#include <vector>
#include "../core/schema.hpp"
#include "row_group.hpp"

namespace columnar {

class ColumnarReader {
public:
    explicit ColumnarReader(const std::filesystem::path& path);
    ~ColumnarReader();

    void loadMetadata();

    const Schema& getSchema() const { return m_schema; }
    size_t getBlockCount() const { return m_blocks.size(); }
    uint64_t getBlockRows(size_t idx) const { return m_blocks[idx].second; }

    RowGroup readBlock(size_t blockIdx) const;

    void forEachRow(std::function<void(const std::vector<std::string>&)> callback) const;

    std::shared_ptr<ColumnData> readColumn(size_t blockIdx, size_t columnIdx) const;

private:
    std::filesystem::path m_path;
    Schema m_schema;
    std::vector<std::pair<uint64_t, uint64_t>> m_blocks; // offset, rows
    bool m_metadataLoaded = false;
};

} // namespace columnar