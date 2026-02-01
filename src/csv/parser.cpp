#include "parser.hpp"

namespace columnar {

std::vector<std::string> Parser::parseLine(std::string_view line, char delimiter) {
    std::vector<std::string> tokens;
    std::string currentToken;
    bool inQuotes = false;
    
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                // Экранированная кавычка
                currentToken += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == delimiter && !inQuotes) {
            tokens.push_back(std::move(currentToken));
            currentToken.clear();
        } else {
            currentToken += c;
        }
    }
    tokens.push_back(std::move(currentToken));
    return tokens;
}

} // namespace columnar