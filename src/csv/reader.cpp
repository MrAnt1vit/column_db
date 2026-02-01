#include "reader.hpp"
#include "parser.hpp"
#include "../utils/file_utils.hpp"
#include <sstream>

namespace columnar {

Schema Reader::readSchema(const std::filesystem::path& path) {
    Schema schema;
    std::string content = read_file(path);
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        if (line.back() == '\r') line.pop_back();

        auto tokens = Parser::parseLine(line, ',');
        
        if (tokens.size() >= 2) {
            schema.addColumn(tokens[0], stringToType(tokens[1]));
        }
        // throw exception if incorrect?
    }
    return schema;
}

std::vector<std::vector<std::string>> Reader::readRows(const std::filesystem::path& path, char delimiter) {
    std::vector<std::vector<std::string>> rows;
    std::string content = read_file(path);
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        if (line.back() == '\r') line.pop_back();

        rows.push_back(Parser::parseLine(line, delimiter));
    }
    return rows;
}

} // namespace columnar