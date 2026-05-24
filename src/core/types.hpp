#pragma once
#include <string_view>
#include <stdexcept>
#include <format>

namespace columnar {

enum class DataType {
    Int8,
    Int16,
    Int32,
    Int64,
    String,
    Date,     
    DateTime,
    Char
};

inline DataType stringToType(std::string_view str) {
    if (str == "Int8") return DataType::Int8;
    if (str == "Int16") return DataType::Int16;
    if (str == "Int32") return DataType::Int32;
    if (str == "Int64") return DataType::Int64;
    if (str == "String") return DataType::String;
    if (str == "Date")      return DataType::Date;
    if (str == "DateTime")  return DataType::DateTime;
    if (str == "Char")      return DataType::Char;
    throw std::runtime_error(std::format("Unknown type: {}", str));
}

} // namespace columnar