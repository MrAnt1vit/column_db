#include "runner.hpp"
#include "../columnar/reader.hpp"
#include "../exec/aggregate.hpp"
#include "../exec/result_writer.hpp"
#include "../exec/scan.hpp"
#include "../exec/filter.hpp"

#include <chrono>
#include <fstream>
#include <memory>
#include <print>
#include <stdexcept>

namespace columnar {

// Q0: SELECT COUNT(*) FROM hits
int q0_countAll(const std::filesystem::path& columnarPath,
                const std::filesystem::path& outputPath) {
    auto reader = std::make_shared<ColumnarReader>(columnarPath);
    reader->loadMetadata();

    std::unique_ptr<IOperator> plan =
        std::make_unique<ScanOperator>(reader, std::vector<size_t>{});
    plan = std::make_unique<GlobalCountOperator>(std::move(plan));

    std::ofstream out(outputPath);
    if (!out) {
        throw std::runtime_error("Cannot open output: " + outputPath.string());
    }

    while (auto batch = plan->Next()) {
        writeBatchToCsv(out, *batch);
    }
    return 0;
}

// Q1: SELECT COUNT(*) FROM hits WHERE AdvEngineID <> 0
int q1_countWhereAdvEngineNonZero(const std::filesystem::path& columnarPath,
                                  const std::filesystem::path& outputPath) {
    auto reader = std::make_shared<ColumnarReader>(columnarPath);
    reader->loadMetadata();

    const Schema& schema = reader->getSchema();
    const auto advIdxOpt = schema.indexOf("AdvEngineID");
    if (!advIdxOpt) {
        throw std::runtime_error("Schema has no AdvEngineID column");
    }
    const size_t advIdx = *advIdxOpt;
    const DataType advType = schema.getColumns()[advIdx].type;

    std::unique_ptr<IFilterExpression> expr;
    switch (advType) {
        case DataType::Int8:
            expr = std::make_unique<ColNotEqualConst<Int8Column>>(0, int8_t{0});
            break;
        case DataType::Int16:
            expr = std::make_unique<ColNotEqualConst<Int16Column>>(0, int16_t{0});
            break;
        case DataType::Int32:
            expr = std::make_unique<ColNotEqualConst<Int32Column>>(0, int32_t{0});
            break;
        case DataType::Int64:
            expr = std::make_unique<ColNotEqualConst<Int64Column>>(0, int64_t{0});
            break;
        default:
            throw std::runtime_error("AdvEngineID has unexpected non-integer type");
    }

    std::unique_ptr<IOperator> plan =
        std::make_unique<ScanOperator>(reader, std::vector<size_t>{advIdx});
    plan = std::make_unique<FilterOperator>(std::move(plan), std::move(expr));
    plan = std::make_unique<GlobalCountOperator>(std::move(plan));

    std::ofstream out(outputPath);
    if (!out) {
        throw std::runtime_error("Cannot open output: " + outputPath.string());
    }
    while (auto batch = plan->Next()) {
        writeBatchToCsv(out, *batch);
    }
    return 0;
}

// SELECT SUM(AdvEngineID), COUNT(*), AVG(ResolutionWidth) FROM hits;
int q2_sumCountAvg(const std::filesystem::path& columnarPath,
                   const std::filesystem::path& outputPath) {
    auto reader = std::make_shared<ColumnarReader>(columnarPath);
    reader->loadMetadata();

    const Schema& schema = reader->getSchema();
    const auto advIdxOpt  = schema.indexOf("AdvEngineID");
    const auto resWIdxOpt = schema.indexOf("ResolutionWidth");
    if (!advIdxOpt)  throw std::runtime_error("Schema has no AdvEngineID");
    if (!resWIdxOpt) throw std::runtime_error("Schema has no ResolutionWidth");

    const size_t advIdx   = *advIdxOpt;
    const size_t resWIdx  = *resWIdxOpt;
    const DataType advType  = schema.getColumns()[advIdx].type;
    const DataType resWType = schema.getColumns()[resWIdx].type;

    std::unique_ptr<IOperator> plan =
        std::make_unique<ScanOperator>(reader,
                                       std::vector<size_t>{advIdx, resWIdx});

    std::vector<AggSpec> specs = {
        { AggSpec::Kind::Sum,   0, advType  },
        { AggSpec::Kind::Count, 0, DataType::Int64 },
        { AggSpec::Kind::Avg,   1, resWType },
    };
    plan = std::make_unique<GlobalAggregateOperator>(std::move(plan),
                                                    std::move(specs));

    std::ofstream out(outputPath);
    if (!out) {
        throw std::runtime_error("Cannot open output: " + outputPath.string());
    }
    while (auto batch = plan->Next()) {
        writeBatchToCsv(out, *batch);
    }
    return 0;
}

// Q3: SELECT AVG(UserID) FROM hits
int q3_avgUserId(const std::filesystem::path& columnarPath,
                 const std::filesystem::path& outputPath) {
    auto reader = std::make_shared<ColumnarReader>(columnarPath);
    reader->loadMetadata();

    const Schema& schema = reader->getSchema();
    const auto userIdIdxOpt = schema.indexOf("UserID");
    if (!userIdIdxOpt) throw std::runtime_error("Schema has no UserID");

    const size_t userIdIdx = *userIdIdxOpt;
    const DataType userIdType = schema.getColumns()[userIdIdx].type;

    std::unique_ptr<IOperator> plan =
        std::make_unique<ScanOperator>(reader,
                                       std::vector<size_t>{userIdIdx});

    std::vector<AggSpec> specs = {
        { AggSpec::Kind::Avg, 0, userIdType },
    };
    plan = std::make_unique<GlobalAggregateOperator>(std::move(plan),
                                                    std::move(specs));

    std::ofstream out(outputPath);
    if (!out) {
        throw std::runtime_error("Cannot open output: " + outputPath.string());
    }
    while (auto batch = plan->Next()) {
        writeBatchToCsv(out, *batch);
    }
    return 0;
}

int runQuery(int queryId,
             const std::filesystem::path& columnarPath,
             const std::filesystem::path& outputPath) {
    const auto t0 = std::chrono::steady_clock::now();

    int rc = 0;
    switch (queryId) {
        case 0:
            rc = q0_countAll(columnarPath, outputPath);
            break;
        case 1:
            rc = q1_countWhereAdvEngineNonZero(columnarPath, outputPath);
            break;
        case 2:
            rc = q2_sumCountAvg(columnarPath, outputPath);
            break;
        case 3:
            rc = q3_avgUserId(columnarPath, outputPath);
            break;
        default:
            std::println(stderr, "Query {} is not implemented yet", queryId);
            std::ofstream out(outputPath, std::ios::binary | std::ios::trunc);
            return 0;  
    }

    const auto t1 = std::chrono::steady_clock::now();
    const double elapsedMs =
        std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::println(stderr, "Query {}: elapsed {:.3f} ms", queryId, elapsedMs);
    return rc;
}

} // namespace columnar