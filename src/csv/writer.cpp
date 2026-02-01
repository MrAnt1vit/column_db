#include "writer.hpp"
#include <stdexcept>

namespace columnar {

Writer::Writer(const std::filesystem::path& path) {
    m_out.open(path);
    if (!m_out) {
        throw std::runtime_error("Cannot open CSV for writing: " + path.string());
    }
}

void Writer::writeRow(const std::vector<std::string>& row) {
    for (size_t i = 0; i < row.size(); ++i) {
        m_out << escape(row[i]);
        if (i < row.size() - 1) {
            m_out << ",";
        }
    }
    m_out << "\n";
}

std::string Writer::escape(const std::string& val) {
    // Если строка содержит " или , или \n, её надо обернуть в кавычки
    bool needsQuotes = false;
    if (val.find_first_of("\",\n") != std::string::npos) {
        needsQuotes = true;
    }

    if (!needsQuotes) {
        return val;
    }

    std::string result = "\"";
    for (char c : val) {
        if (c == '"') {
            result += "\"\""; // Экранируем кавычку
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

} // namespace columnar