#include "columnar_db.hpp" // Подключаем только один файл!
#include <print>

using namespace columnar;

int main(int argc, char* argv[]) {
    try {
        std::string csvPath = "data.csv";
        std::string schemaPath = "schema.csv";
        std::string dbPath = "output.columnar";
        
        if (argc >= 3) {
            csvPath = argv[1];
            schemaPath = argv[2];
        }
        if (argc >= 4) {
            dbPath = argv[3];
        }

        const std::string restoredCsvPath = "data_restored.csv";

        // Конвертация в БД
        std::println("Converting '{}' using schema '{}'...", csvPath, schemaPath);
        columnar::Converter::csvToColumnar(csvPath, schemaPath, dbPath);
        std::println("Saved to '{}'", dbPath);

        // Конвертация обратно
        std::println("Restoring back to CSV...");
        columnar::Converter::columnarToCsv(dbPath, restoredCsvPath);
        std::println("Restored to '{}'", restoredCsvPath);

    } catch (const std::exception& e) {
        std::println(stderr, "Error: {}", e.what());
        return 1;
    }
    return 0;
}