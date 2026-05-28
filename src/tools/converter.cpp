#include "converter.hpp"
#include "../columnar/writer.hpp"
#include "../columnar/reader.hpp"
#include "../csv/reader.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>

namespace columnar {

void Converter::csvToColumnar(const std::filesystem::path& csvPath,
                              const std::filesystem::path& schemaPath,
                              const std::filesystem::path& outputPath,
                              char delimiter) {
    auto schema = Reader::readSchema(schemaPath);
    ColumnarWriter writer(outputPath, schema);

    std::error_code ec;
    uint64_t totalBytes = std::filesystem::file_size(csvPath, ec);
    if (ec || totalBytes == 0) totalBytes = 1;

    const size_t BLOCK_BYTES = 128 * 1024 * 1024;
    RowGroup currentBlock(schema, BLOCK_BYTES);

    size_t processedRows = 0;
    uint64_t totalBytesRead = 0;

    auto startTime = std::chrono::steady_clock::now();
    auto lastProgressTime = startTime;

    Reader::readRows(csvPath, delimiter, [&](const std::vector<std::string>& row) {
        for (const auto& field : row) {
            totalBytesRead += field.size() + 1;
        }

        currentBlock.addRow(row);
        ++processedRows;

        auto now = std::chrono::steady_clock::now();
        if (now - lastProgressTime >= std::chrono::seconds(1)) {
            double elapsed = std::chrono::duration<double>(now - startTime).count();
            double percent = 100.0 * static_cast<double>(totalBytesRead)
                                  / static_cast<double>(totalBytes);
            if (percent > 100.0) percent = 100.0;
            double rowsPerSec = processedRows / elapsed;
            double bytesPerSec = totalBytesRead / elapsed;
            double mbPerSec = bytesPerSec / (1024.0 * 1024.0);
            double eta = (bytesPerSec > 0)
                         ? std::max(0.0,
                                    (static_cast<double>(totalBytes) - totalBytesRead) / bytesPerSec)
                         : 0.0;

            std::cout << "\r\x1b[K"
                      << "Processed " << processedRows << " rows ("
                      << std::fixed << std::setprecision(1) << percent << "%), "
                      << std::setprecision(0) << rowsPerSec << " rows/s, "
                      << std::setprecision(2) << mbPerSec << " MB/s, ETA "
                      << std::setprecision(0) << eta << "s"
                      << std::flush;
            lastProgressTime = now;
        }

        if (currentBlock.isFull()) {
            writer.writeBlock(currentBlock);
            currentBlock.clear();
        }
    }, schema.size());

    if (currentBlock.rowCount() > 0) {
        writer.writeBlock(currentBlock);
    }
    writer.finalize();

    auto endTime = std::chrono::steady_clock::now();
    double totalSec = std::chrono::duration<double>(endTime - startTime).count();
    std::cout << "\nConversion finished. Processed " << processedRows << " rows in "
              << totalSec << " seconds ("
              << std::fixed << std::setprecision(0) << (processedRows / totalSec)
              << " rows/s).\n";
}

void Converter::columnarToCsv(const std::filesystem::path& columnarPath,
                              const std::filesystem::path& csvOutputPath,
                              char delimiter) {
    ColumnarReader reader(columnarPath);
    reader.loadMetadata();

    std::ofstream out(csvOutputPath);
    if (!out) throw std::runtime_error("Cannot create CSV file");

    size_t totalRows = 0;
    for (size_t i = 0; i < reader.getBlockCount(); ++i) {
        totalRows += reader.getBlockRows(i);
    }

    size_t processedRows = 0;
    size_t totalBytesWritten = 0;
    auto startTime = std::chrono::steady_clock::now();
    auto lastProgress = startTime;

    reader.forEachRow([&](const std::vector<std::string>& row) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) out << delimiter;
            out << row[i];
            totalBytesWritten += row[i].size();
        }
        out << '\n';
        totalBytesWritten += 1;
        ++processedRows;

        auto now = std::chrono::steady_clock::now();
        if (now - lastProgress >= std::chrono::seconds(1)) {
            double elapsed = std::chrono::duration<double>(now - startTime).count();
            double percent = 100.0 * processedRows / totalRows;
            double rowsPerSec = processedRows / elapsed;
            double mbPerSec = (totalBytesWritten / 1024.0 / 1024.0) / elapsed;
            double eta = (totalRows - processedRows) / rowsPerSec;
            std::cout << "\r\x1b[K"
                      << "Exported " << processedRows << " / " << totalRows << " rows ("
                      << std::fixed << std::setprecision(1) << percent << "%), "
                      << std::setprecision(0) << rowsPerSec << " rows/s, "
                      << std::setprecision(2) << mbPerSec << " MB/s, ETA "
                      << std::setprecision(0) << eta << "s"
                      << std::flush;
            lastProgress = now;
        }
    });

    auto endTime = std::chrono::steady_clock::now();
    double totalSec = std::chrono::duration<double>(endTime - startTime).count();
    std::cout << "\r\x1b[K"
              << "Export finished. Exported " << processedRows << " rows in "
              << totalSec << " seconds ("
              << std::fixed << std::setprecision(0) << (processedRows / totalSec)
              << " rows/s).\n";
}

} // namespace columnar