#pragma once
#include <string_view>
#include <stdexcept>
#include <format>

namespace columnar {

enum class DataType {
    INT64,
    STRING
};

constexpr std::string_view typeToString(DataType type) {
    switch (type) {
        case DataType::INT64: return "int64";
        case DataType::STRING: return "string";
    }
    return "unknown";
}

inline DataType stringToType(std::string_view str) {
    if (str == "int64") return DataType::INT64;
    if (str == "string") return DataType::STRING;
    throw std::runtime_error(std::format("Unknown type: {}", str));
}

} // namespace columnar