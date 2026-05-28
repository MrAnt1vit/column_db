#include "columnar_db.hpp"
#include "query/runner.hpp"

#include <format>
#include <print>
#include <string>
#include <string_view>

using namespace columnar;

namespace {

int doConvert(const std::string& csvPath,
              const std::string& schemaPath,
              const std::string& dbPath) {
    const char delimiter = csvPath.ends_with(".tsv") ? '\t' : ',';
    std::println("Converting '{}' using schema '{}'...", csvPath, schemaPath);
    Converter::csvToColumnar(csvPath, schemaPath, dbPath, delimiter);
    std::println("Saved to '{}'", dbPath);
    return 0;
}

int doRestore(const std::string& dbPath, const std::string& csvOutPath) {
    std::println("Restoring '{}' back to CSV at '{}'...", dbPath, csvOutPath);
    Converter::columnarToCsv(dbPath, csvOutPath);
    return 0;
}

int runDevPipeline() {
    const std::string csvPath    = "hits_small.csv";
    const std::string schemaPath = "schema_cpp.csv";
    const std::string dbPath     = "output.columnar";
    const std::string outPath    = "data_restored.csv";

    doConvert(csvPath, schemaPath, dbPath);
    // doRestore(dbPath, outPath);

    constexpr int kFirstQuery = 0;
    constexpr int kLastQuery  = 42;
    for (int q = kFirstQuery; q <= kLastQuery; ++q) {
        const std::string outPath = std::format("query_{}.csv", q);
        runQuery(q, dbPath, outPath);
    }
    return 0;
}

int parseConvert(int argc, char* argv[]) {
    if (argc < 5) {
        std::println(stderr,
                     "Usage: {} convert <csv> <schema> <columnar>", argv[0]);
        return 1;
    }
    return doConvert(argv[2], argv[3], argv[4]);
}

int parseRestore(int argc, char* argv[]) {
    if (argc < 4) {
        std::println(stderr,
                     "Usage: {} restore <columnar> <csv>", argv[0]);
        return 1;
    }
    return doRestore(argv[2], argv[3]);
}

int parseQuery(int argc, char* argv[]) {
    if (argc < 5) {
        std::println(stderr,
                     "Usage: {} query <query_id> <columnar> <output_csv>",
                     argv[0]);
        return 1;
    }
    return runQuery(std::stoi(argv[2]), argv[3], argv[4]);
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            return runDevPipeline();
        }

        const std::string_view command = argv[1];
        if (command == "convert") return parseConvert(argc, argv);
        if (command == "restore") return parseRestore(argc, argv);
        if (command == "query")   return parseQuery(argc, argv);

        std::println(stderr,
                     "Unknown subcommand: '{}'. Expected 'convert', 'restore' or 'query'.",
                     command);
        return 1;
    } catch (const std::exception& e) {
        std::println(stderr, "Error: {}", e.what());
        return 1;
    }
}