#include "parser.hpp"
#include <string>

namespace columnar {
static std::string pendingBuffer;
static bool pendingIncomplete = false;

void Parser::resetBuffer() {
    pendingBuffer.clear();
    pendingIncomplete = false;
}

std::vector<std::string> Parser::parseLine(const std::string& line, char delimiter) {
    if (!pendingIncomplete) {
        pendingBuffer = line;
    } else {
        pendingBuffer += '\n' + line;
    }

    bool inside = false;
    for (size_t i = 0; i < pendingBuffer.size(); ++i) {
        if (pendingBuffer[i] == '"') {
            if (i + 1 < pendingBuffer.size() && pendingBuffer[i+1] == '"') {
                ++i;
            } else {
                inside = !inside;
            }
        }
    }
    if (inside) {
        pendingIncomplete = true;
        return {};
    }

    std::vector<std::string> tokens;
    std::string currentToken;
    bool inQuotes = false;
    for (size_t i = 0; i < pendingBuffer.size(); ++i) {
        char c = pendingBuffer[i];
        if (c == '"') {
            if (inQuotes && i + 1 < pendingBuffer.size() && pendingBuffer[i+1] == '"') {
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

    pendingBuffer.clear();
    pendingIncomplete = false;
    return tokens;
}
} // namespace columnar