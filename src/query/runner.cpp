#include "runner.hpp"
#include "../columnar/reader.hpp"
#include "../exec/aggregate.hpp"
#include "../exec/result_writer.hpp"
#include "../exec/scan.hpp"
#include "../exec/filter.hpp"
#include "../exec/count_distinct.hpp"
#include "../exec/hash_aggregate.hpp"
#include "../exec/sort.hpp"
#include "../exec/limit.hpp"



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

// Q4: SELECT COUNT(DISTINCT UserID) FROM hits
int q4_countDistinctUserId(const std::filesystem::path& columnarPath,
                           const std::filesystem::path& outputPath) {
    auto reader = std::make_shared<ColumnarReader>(columnarPath);
    reader->loadMetadata();

    const Schema& schema = reader->getSchema();
    const auto userIdIdxOpt = schema.indexOf("UserID");
    if (!userIdIdxOpt) throw std::runtime_error("Schema has no UserID");

    const size_t   userIdIdx  = *userIdIdxOpt;
    const DataType userIdType = schema.getColumns()[userIdIdx].type;

    std::unique_ptr<IOperator> plan =
        std::make_unique<ScanOperator>(reader, std::vector<size_t>{userIdIdx});
    plan = std::make_unique<CountDistinctOperator>(std::move(plan),
                                                   0, userIdType);

    std::ofstream out(outputPath);
    if (!out) {
        throw std::runtime_error("Cannot open output: " + outputPath.string());
    }
    while (auto batch = plan->Next()) {
        writeBatchToCsv(out, *batch);
    }
    return 0;
}

// Q5: SELECT COUNT(DISTINCT SearchPhrase) FROM hits
int q5_countDistinctSearchPhrase(const std::filesystem::path& columnarPath,
                                 const std::filesystem::path& outputPath) {
    auto reader = std::make_shared<ColumnarReader>(columnarPath);
    reader->loadMetadata();

    const Schema& schema = reader->getSchema();
    const auto spIdxOpt = schema.indexOf("SearchPhrase");
    if (!spIdxOpt) throw std::runtime_error("Schema has no SearchPhrase");

    const size_t   spIdx  = *spIdxOpt;
    const DataType spType = schema.getColumns()[spIdx].type;

    std::unique_ptr<IOperator> plan =
        std::make_unique<ScanOperator>(reader, std::vector<size_t>{spIdx});
    plan = std::make_unique<CountDistinctOperator>(std::move(plan),
                                                   0, spType);

    std::ofstream out(outputPath);
    if (!out) {
        throw std::runtime_error("Cannot open output: " + outputPath.string());
    }
    while (auto batch = plan->Next()) {
        writeBatchToCsv(out, *batch);
    }
    return 0;
}

// Q6: SELECT MIN(EventDate), MAX(EventDate) FROM hits
int q6_minMaxEventDate(const std::filesystem::path& columnarPath,
                       const std::filesystem::path& outputPath) {
    auto reader = std::make_shared<ColumnarReader>(columnarPath);
    reader->loadMetadata();

    const Schema& schema = reader->getSchema();
    const auto idxOpt = schema.indexOf("EventDate");
    if (!idxOpt) throw std::runtime_error("Schema has no EventDate");

    const size_t   idx  = *idxOpt;
    const DataType type = schema.getColumns()[idx].type;

    std::unique_ptr<IOperator> plan =
        std::make_unique<ScanOperator>(reader, std::vector<size_t>{idx});

    std::vector<AggSpec> specs = {
        { AggSpec::Kind::Min, 0, type },
        { AggSpec::Kind::Max, 0, type },
    };
    plan = std::make_unique<GlobalAggregateOperator>(std::move(plan),
                                                    std::move(specs));

    std::ofstream out(outputPath);
    if (!out) throw std::runtime_error("Cannot open output: " + outputPath.string());
    while (auto batch = plan->Next()) writeBatchToCsv(out, *batch);
    return 0;
}

// Q7: SELECT AdvEngineID, COUNT(*) FROM hits
//     WHERE AdvEngineID <> 0 GROUP BY AdvEngineID ORDER BY COUNT(*) DESC
int q7_groupCountAdvEngine(const std::filesystem::path& columnarPath,
                           const std::filesystem::path& outputPath) {
    auto reader = std::make_shared<ColumnarReader>(columnarPath);
    reader->loadMetadata();

    const Schema& schema = reader->getSchema();
    const auto advIdxOpt = schema.indexOf("AdvEngineID");
    if (!advIdxOpt) throw std::runtime_error("Schema has no AdvEngineID");

    const size_t   advIdx  = *advIdxOpt;
    const DataType advType = schema.getColumns()[advIdx].type;

    std::unique_ptr<IFilterExpression> expr;
    switch (advType) {
        case DataType::Int8:
            expr = std::make_unique<ColNotEqualConst<Int8Column>>(0, int8_t{0}); break;
        case DataType::Int16:
            expr = std::make_unique<ColNotEqualConst<Int16Column>>(0, int16_t{0}); break;
        case DataType::Int32:
            expr = std::make_unique<ColNotEqualConst<Int32Column>>(0, int32_t{0}); break;
        case DataType::Int64:
            expr = std::make_unique<ColNotEqualConst<Int64Column>>(0, int64_t{0}); break;
        default:
            throw std::runtime_error("AdvEngineID has unexpected non-integer type");
    }

    std::unique_ptr<IOperator> plan =
    std::make_unique<ScanOperator>(reader, std::vector<size_t>{advIdx});
    plan = std::make_unique<FilterOperator>(std::move(plan),
            std::make_unique<ColNotEqualConst<Int16Column>>( 0,  0));

    std::vector<AggSpec> specs = {
        { AggSpec::Kind::Count, 0, DataType::Int64 },
    };
    plan = std::make_unique<HashGroupByAggregateOperator>(
        std::move(plan), 0, advType, std::move(specs));

    plan = std::make_unique<SortOperator>(
    std::move(plan), std::vector<SortKey>{ {1, false} });

    std::ofstream out(outputPath);
    if (!out) throw std::runtime_error("Cannot open output: " + outputPath.string());
    while (auto batch = plan->Next()) writeBatchToCsv(out, *batch);
    return 0;
}

// Q8: SELECT RegionID, COUNT(DISTINCT UserID) AS u FROM hits
//     GROUP BY RegionID ORDER BY u DESC LIMIT 10
int q8_regionDistinctUsers(const std::filesystem::path& columnarPath,
                           const std::filesystem::path& outputPath) {
    auto reader = std::make_shared<ColumnarReader>(columnarPath);
    reader->loadMetadata();

    const Schema& schema = reader->getSchema();
    const auto regionIdxOpt = schema.indexOf("RegionID");
    const auto userIdxOpt   = schema.indexOf("UserID");
    if (!regionIdxOpt) throw std::runtime_error("Schema has no RegionID");
    if (!userIdxOpt)   throw std::runtime_error("Schema has no UserID");

    const size_t   regionIdx  = *regionIdxOpt;
    const size_t   userIdx    = *userIdxOpt;
    const DataType regionType = schema.getColumns()[regionIdx].type;
    const DataType userType   = schema.getColumns()[userIdx].type;

    std::unique_ptr<IOperator> plan =
    std::make_unique<ScanOperator>(reader, std::vector<size_t>{regionIdx, userIdx});

    std::vector<AggSpec> specs = {
        { AggSpec::Kind::CountDistinct, 1, userType },
    };
    plan = std::make_unique<HashGroupByAggregateOperator>(
        std::move(plan), 0, regionType, std::move(specs));

    plan = std::make_unique<SortOperator>(
        std::move(plan), std::vector<SortKey>{ {1, false} });
    plan = std::make_unique<LimitOperator>(std::move(plan), 10);

    std::ofstream out(outputPath);
    if (!out) throw std::runtime_error("Cannot open output: " + outputPath.string());
    while (auto batch = plan->Next()) writeBatchToCsv(out, *batch);
    return 0;
}

// Q9: SELECT RegionID, SUM(AdvEngineID), COUNT(*) AS c,
//            AVG(ResolutionWidth), COUNT(DISTINCT UserID)
//     FROM hits GROUP BY RegionID ORDER BY c DESC LIMIT 10
int q9_regionMultiAgg(const std::filesystem::path& columnarPath,
                      const std::filesystem::path& outputPath) {
    auto reader = std::make_shared<ColumnarReader>(columnarPath);
    reader->loadMetadata();

    const Schema& schema = reader->getSchema();
    auto regionIdxOpt = schema.indexOf("RegionID");
    auto advIdxOpt    = schema.indexOf("AdvEngineID");
    auto resWIdxOpt   = schema.indexOf("ResolutionWidth");
    auto userIdxOpt   = schema.indexOf("UserID");
    if (!regionIdxOpt || !advIdxOpt || !resWIdxOpt || !userIdxOpt)
        throw std::runtime_error("Q9: missing required column in schema");

    const size_t regionIdx = *regionIdxOpt;
    const size_t advIdx    = *advIdxOpt;
    const size_t resWIdx   = *resWIdxOpt;
    const size_t userIdx   = *userIdxOpt;

    const DataType regionType = schema.getColumns()[regionIdx].type;
    const DataType advType    = schema.getColumns()[advIdx].type;
    const DataType resWType   = schema.getColumns()[resWIdx].type;
    const DataType userType   = schema.getColumns()[userIdx].type;

    std::unique_ptr<IOperator> plan =
        std::make_unique<ScanOperator>(reader,
            std::vector<size_t>{regionIdx, advIdx, resWIdx, userIdx});

    std::vector<AggSpec> specs = {
        { AggSpec::Kind::Sum,            1, advType  },
        { AggSpec::Kind::Count,           0, DataType::Int64 },
        { AggSpec::Kind::Avg,            2, resWType },
        { AggSpec::Kind::CountDistinct,  3, userType },
    };

    plan = std::make_unique<HashGroupByAggregateOperator>(
        std::move(plan), 0, regionType, std::move(specs));

    plan = std::make_unique<SortOperator>(
        std::move(plan),
        std::vector<SortKey>{ { 2, false} });
    plan = std::make_unique<LimitOperator>(std::move(plan), 10);

    std::ofstream out(outputPath);
    if (!out) throw std::runtime_error("Cannot open output: " + outputPath.string());
    while (auto batch = plan->Next()) writeBatchToCsv(out, *batch);
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
        case 4:
            rc = q4_countDistinctUserId(columnarPath, outputPath);
            break;
        case 5:
            rc = q5_countDistinctSearchPhrase(columnarPath, outputPath);
            break;
        case 6:
            rc = q6_minMaxEventDate(columnarPath, outputPath);
            break;
        case 7:
            rc = q7_groupCountAdvEngine(columnarPath, outputPath);
            break;
        case 8:
            rc = q8_regionDistinctUsers(columnarPath, outputPath);
            break;
        case 9:
            rc = q9_regionMultiAgg(columnarPath, outputPath);
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