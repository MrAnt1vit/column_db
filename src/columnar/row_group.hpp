#pragma once
#include <vector>
#include <string>
#include <memory>
#include "../core/types.hpp"

namespace columnar {

// Базовый интерфейс колонки
class ColumnData {
public:
    virtual ~ColumnData() = default;
    virtual DataType getType() const = 0;
    virtual size_t size() const = 0;
};

// Колонка чисел
class Int64Column : public ColumnData {
public:
    std::vector<int64_t> data;
    
    DataType getType() const override { return DataType::INT64; }
    size_t size() const override { return data.size(); }
    
    void add(int64_t val) { data.push_back(val); }
};

// Колонка строк
class StringColumn : public ColumnData {
public:
    std::vector<std::string> data;
    
    DataType getType() const override { return DataType::STRING; }
    size_t size() const override { return data.size(); }
    
    void add(std::string val) { data.push_back(std::move(val)); }
};

class RowGroup {
public:
    std::vector<std::shared_ptr<ColumnData>> columns;
    size_t rowCount = 0;

    RowGroup(const class Schema& schema);
};

} // namespace columnar