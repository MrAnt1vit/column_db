#include "columnar_db.hpp"
#include "tools/csv_compare.hpp"
#include <print>

using namespace columnar;

int main(int argc, char* argv[]) {
    try {
        std::string csvPath = "hits_small.csv";
        std::string schemaPath = "schema_cpp.csv";
        std::string dbPath = "output.columnar";
        
        if (argc >= 3) {
            csvPath = argv[1];
            schemaPath = argv[2];
        }
        if (argc >= 4) {
            dbPath = argv[3];
        }

        const std::string restoredCsvPath = "data_restored.csv";

        char delimiter = ',';
        if (csvPath.ends_with("tsv")) {
            delimiter = '\t';
        }

        std::println("Converting '{}' using schema '{}'...", csvPath, schemaPath);
        Converter::csvToColumnar(csvPath, schemaPath, dbPath, delimiter);
        std::println("Saved to '{}'", dbPath);

        std::println("Restoring back to CSV...");
        Converter::columnarToCsv(dbPath, restoredCsvPath);
        std::println("Restored to '{}'", restoredCsvPath);

        // std::println("Validating restored CSV against original (logical comparison)...");
        // CsvDiff diff;

        // bool identical = compareCsvFilesLogical(csvPath, restoredCsvPath, delimiter, &diff, true);        if (identical) {
        //     std::println("✅ SUCCESS: Original and restored CSV files are identical.");
        // } else {
        //     std::println(stderr, "❌ FAILURE: Files differ at logical row {}, column {} (0-based).", diff.line, diff.column);
        //     std::println(stderr, "   Original field: '{}'", diff.left);
        //     std::println(stderr, "   Restored field: '{}'", diff.right);
        //     return 1;
        // }

    } catch (const std::exception& e) {
        std::println(stderr, "Error: {}", e.what());
        return 1;
    }
    return 0;
}