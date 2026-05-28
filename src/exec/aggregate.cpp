#include "aggregate.hpp"
#include "../columnar/row_group.hpp"

#include <stdexcept>

namespace columnar {

GlobalCountOperator::GlobalCountOperator(std::unique_ptr<IOperator> child)
    : m_child(std::move(child))
{}

std::optional<Batch> GlobalCountOperator::Next() {
    if (m_done) return std::nullopt;
    m_done = true;

    int64_t total = 0;
    while (auto batch = m_child->Next()) {
        total += static_cast<int64_t>(batch->rowCount);
    }

    auto col = std::make_shared<Int64Column>();
    col->data.push_back(total);

    Batch result;
    result.rowCount = 1;
    result.columns.push_back(std::move(col));
    return result;
}

namespace {

template<typename ColumnT>
i128 sumColumnAsInt128(const ColumnData& col, size_t rowCount) {
    const auto* typed = static_cast<const ColumnT*>(&col);
    const auto* data = typed->data.data();
    i128 total = 0;
    for (size_t i = 0; i < rowCount; ++i) {
        total += static_cast<i128>(data[i]);
    }
    return total;
}

i128 sumByType(DataType type, const ColumnData& col, size_t rowCount) {
    switch (type) {
        case DataType::Int8:  return sumColumnAsInt128<Int8Column>(col, rowCount);
        case DataType::Int16: return sumColumnAsInt128<Int16Column>(col, rowCount);
        case DataType::Int32: return sumColumnAsInt128<Int32Column>(col, rowCount);
        case DataType::Int64: return sumColumnAsInt128<Int64Column>(col, rowCount);
        default:
            throw std::runtime_error("Sum/Avg: unsupported column type");
    }
}

} // namespace

GlobalAggregateOperator::GlobalAggregateOperator(
    std::unique_ptr<IOperator> child,
    std::vector<AggSpec> specs)
    : m_child(std::move(child))
    , m_specs(std::move(specs))
    , m_states(m_specs.size())
{}

std::optional<Batch> GlobalAggregateOperator::Next() {
    if (m_done) return std::nullopt;
    m_done = true;

    while (auto batch = m_child->Next()) {
        if (batch->rowCount == 0) continue;

        for (size_t i = 0; i < m_specs.size(); ++i) {
            const auto& spec = m_specs[i];
            auto& st = m_states[i];

            switch (spec.kind) {
                case AggSpec::Kind::Count: {
                    st.count += static_cast<int64_t>(batch->rowCount);
                    break;
                }

                case AggSpec::Kind::Sum: {
                    if (spec.batchColIdx >= batch->columns.size()) {
                        throw std::runtime_error("Sum: column not in batch");
                    }
                    st.accum += sumByType(spec.srcType,
                                          *batch->columns[spec.batchColIdx],
                                          batch->rowCount);
                    break;
                }

                case AggSpec::Kind::Avg: {
                    if (spec.batchColIdx >= batch->columns.size()) {
                        throw std::runtime_error("Avg: column not in batch");
                    }
                    st.accum += sumByType(spec.srcType,
                                          *batch->columns[spec.batchColIdx],
                                          batch->rowCount);
                    st.count += static_cast<int64_t>(batch->rowCount);
                    break;
                }
            }
        }
    }

    Batch result;
    result.rowCount = 1;
    result.columns.reserve(m_specs.size());

    for (size_t i = 0; i < m_specs.size(); ++i) {
        const auto& spec = m_specs[i];
        const auto& st = m_states[i];

        switch (spec.kind) {
            case AggSpec::Kind::Count: {
                auto col = std::make_shared<Int64Column>();
                col->data.push_back(st.count);
                result.columns.push_back(std::move(col));
                break;
            }
            case AggSpec::Kind::Sum: {
                auto col = std::make_shared<Int128Column>();
                col->data.push_back(st.accum);
                result.columns.push_back(std::move(col));
                break;
            }
            case AggSpec::Kind::Avg: {
                auto col = std::make_shared<Int64Column>();
                const i128 avg128 = (st.count == 0)
                    ? i128{0}
                    : (st.accum / static_cast<i128>(st.count));
                col->data.push_back(static_cast<int64_t>(avg128));
                result.columns.push_back(std::move(col));
                break;
            }
        }
    }
    return result;
}

} // namespace columnar