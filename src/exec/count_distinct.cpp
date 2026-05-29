#include "count_distinct.hpp"
#include "../columnar/row_group.hpp"

#include <stdexcept>
#include <string>
#include <unordered_set>

namespace columnar {

namespace {

template<typename ColumnT>
static int64_t countDistinctTyped(IOperator& child, size_t colIdx) {
    using V = typename ColumnT::value_type;
    std::unordered_set<V> seen;
    while (auto batch = child.Next()) {
        if (batch->rowCount == 0) continue;
        if (colIdx >= batch->columns.size()) {
            throw std::runtime_error("CountDistinct: column not in batch");
        }
        const auto* col =
            static_cast<const ColumnT*>(batch->columns[colIdx].get());
        const auto* data = col->data.data();
        seen.reserve(seen.size() + batch->rowCount);
        for (size_t i = 0; i < batch->rowCount; ++i) {
            seen.insert(data[i]);
        }
    }
    return static_cast<int64_t>(seen.size());
}

static int64_t countDistinctString(IOperator& child, size_t colIdx) {
    std::unordered_set<std::string> seen;
    while (auto batch = child.Next()) {
        if (batch->rowCount == 0) continue;
        if (colIdx >= batch->columns.size()) {
            throw std::runtime_error("CountDistinct: column not in batch");
        }
        const auto* col =
            static_cast<const StringColumn*>(batch->columns[colIdx].get());
        seen.reserve(seen.size() + batch->rowCount);
        for (size_t i = 0; i < batch->rowCount; ++i) {
            seen.insert(col->data[i]);
        }
    }
    return static_cast<int64_t>(seen.size());
}

} // namespace

CountDistinctOperator::CountDistinctOperator(std::unique_ptr<IOperator> child,
                                             size_t batchColIdx,
                                             DataType srcType)
    : m_child(std::move(child))
    , m_batchColIdx(batchColIdx)
    , m_srcType(srcType)
{}

std::optional<Batch> CountDistinctOperator::Next() {
    if (m_done) return std::nullopt;
    m_done = true;

    int64_t distinctCount = 0;
    switch (m_srcType) {
        case DataType::Int8:
            distinctCount = countDistinctTyped<Int8Column>(*m_child, m_batchColIdx);
            break;
        case DataType::Int16:
            distinctCount = countDistinctTyped<Int16Column>(*m_child, m_batchColIdx);
            break;
        case DataType::Int32:
            distinctCount = countDistinctTyped<Int32Column>(*m_child, m_batchColIdx);
            break;
        case DataType::Int64:
            distinctCount = countDistinctTyped<Int64Column>(*m_child, m_batchColIdx);
            break;
        case DataType::Date:
            distinctCount = countDistinctTyped<DateColumn>(*m_child, m_batchColIdx);
            break;
        case DataType::DateTime:
            distinctCount = countDistinctTyped<DateTimeColumn>(*m_child, m_batchColIdx);
            break;
        case DataType::Char:
            distinctCount = countDistinctTyped<CharColumn>(*m_child, m_batchColIdx);
            break;
        case DataType::String:
            distinctCount = countDistinctString(*m_child, m_batchColIdx);
            break;
        default:
            throw std::runtime_error("CountDistinct: unsupported column type");
    }

    auto col = std::make_shared<Int64Column>();
    col->data.push_back(distinctCount);

    Batch result;
    result.rowCount = 1;
    result.columns.push_back(std::move(col));
    return result;
}

} // namespace columnar