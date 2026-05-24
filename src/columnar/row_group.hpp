#pragma once
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <ostream>
#include <istream>
#include <ctime>
#include <iomanip>
#include "../core/schema.hpp"

namespace columnar {

class ColumnData {
public:
    virtual ~ColumnData() = default;
    virtual DataType getType() const = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;
    virtual size_t addFromString(const std::string& str) = 0;
    virtual void serialize(std::ostream& out) const = 0;
    virtual void deserialize(std::istream& in, size_t rows) = 0;
    virtual std::string getString(size_t index) const = 0;
};

template<typename T, DataType DT>
class TypedColumn : public ColumnData {
public:
    std::vector<T> data;

    DataType getType() const override { return DT; }
    size_t size() const override { return data.size(); }
    void clear() override { data.clear(); }

    size_t addFromString(const std::string& str) override {
        T val = 0;
        if (!str.empty()) {
            try {
                if constexpr (std::is_unsigned_v<T>) {
                    val = static_cast<T>(std::stoull(str));
                } else {
                    val = static_cast<T>(std::stoll(str));
                }
            } catch (const std::exception&) {
                // Ошибка преобразования — оставляем 0
            }
        }
        data.push_back(val);
        return sizeof(T);
    }

    void serialize(std::ostream& out) const override {
        out.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(T));
    }

    void deserialize(std::istream& in, size_t rows) override {
        data.resize(rows);
        in.read(reinterpret_cast<char*>(data.data()), rows * sizeof(T));
    }

    std::string getString(size_t index) const override {
        return std::to_string(data[index]);
    }
};

class CharColumn : public ColumnData {
public:
    std::vector<int8_t> data;
    static constexpr int8_t NULL_SENTINEL = -128;

    DataType getType() const override { return DataType:: Char; }
    size_t size() const override { return data.size(); }
    void clear() override { data.clear(); }

    size_t addFromString(const std::string& str) override {
        if (str.empty()) {
            data.push_back(NULL_SENTINEL);
            return sizeof(int8_t);
        }
        std::string_view sv = str;
        char ch = sv[0];
        data.push_back(static_cast<int8_t>(ch));
        return sizeof(int8_t);
    }

    void serialize(std::ostream& out) const override {
        out.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(int8_t));
    }

    void deserialize(std::istream& in, size_t rows) override {
        data.resize(rows);
        in.read(reinterpret_cast<char*>(data.data()), rows * sizeof(int8_t));
    }

    std::string getString(size_t index) const override {
        int8_t val = data[index];
        if (val == NULL_SENTINEL) {
            return "\"\"";
        }
        char ch = static_cast<char>(val);
        return "\"" + std::string(1, ch) + "\"";
    }
};

class StringColumn : public ColumnData {
public:
    std::vector<std::string> data;

    DataType getType() const override { return DataType::String; }
    size_t size() const override { return data.size(); }
    void clear() override { data.clear(); }

    size_t addFromString(const std::string& str) override {
        data.push_back(str);
        return sizeof(uint32_t) + str.size();
    }

    void serialize(std::ostream& out) const override {
        for (const auto& s : data) {
            uint32_t len = static_cast<uint32_t>(s.size());
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(s.data(), len);
        }
    }

    void deserialize(std::istream& in, size_t rows) override {
        data.clear();
        data.reserve(rows);
        for (size_t i = 0; i < rows; ++i) {
            uint32_t len;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            std::string s(len, '\0');
            in.read(s.data(), len);
            data.push_back(std::move(s));
        }
    }

    std::string getString(size_t index) const override {
        return "\"" + data[index] + "\"";
    }
};

namespace detail {
    inline std::time_t utc_mktime(const std::tm& tm) {
        return timegm(const_cast<std::tm*>(&tm));
    }

    inline int32_t days_from_epoch(int year, int month, int day) {
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
        std::time_t t = utc_mktime(tm);
        return static_cast<int32_t>(t / 86400);
    }

    inline std::string format_date(int32_t days) {
        std::time_t t = static_cast<std::time_t>(days) * 86400;
        std::tm tm = *std::gmtime(&t);
        std::ostringstream oss;
        oss << "\"";
        oss << std::put_time(&tm, "%Y-%m-%d");
        oss << "\"";
        return oss.str();
    }
}

class DateColumn : public ColumnData {
public:
    std::vector<int32_t> data;

    DataType getType() const override { return DataType::Date; }
    size_t size() const override { return data.size(); }
    void clear() override { data.clear(); }

    size_t addFromString(const std::string& str) override {
        if (str.empty()) {
            data.push_back(0);
            return sizeof(int32_t);
        }
        std::tm tm = {};
        std::istringstream ss(str);
        ss >> std::get_time(&tm, "%Y-%m-%d");
        if (ss.fail()) {
            throw std::runtime_error("Invalid Date format: " + str);
        }
        int32_t days = detail::days_from_epoch(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
        data.push_back(days);
        return sizeof(int32_t);
    }

    void serialize(std::ostream& out) const override {
        out.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(int32_t));
    }

    void deserialize(std::istream& in, size_t rows) override {
        data.resize(rows);
        in.read(reinterpret_cast<char*>(data.data()), rows * sizeof(int32_t));
    }

    std::string getString(size_t index) const override {
        return detail::format_date(data[index]);
    }
};

class DateTimeColumn : public ColumnData {
public:
    std::vector<int64_t> data;
    DataType getType() const override { return DataType::DateTime; }
    size_t size() const override { return data.size(); }
    void clear() override { data.clear(); }

    size_t addFromString(const std::string& str) override {
        if (str.empty()) {
            data.push_back(0);
            return sizeof(int64_t);
        }
        std::tm tm = {};
        std::istringstream ss(str);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (ss.fail()) {
            throw std::runtime_error("Invalid DateTime format: " + str);
        }
        std::time_t t = detail::utc_mktime(tm);
        data.push_back(static_cast<int64_t>(t));
        return sizeof(int64_t);
    }

    void serialize(std::ostream& out) const override {
        out.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(int64_t));
    }

    void deserialize(std::istream& in, size_t rows) override {
        data.resize(rows);
        in.read(reinterpret_cast<char*>(data.data()), rows * sizeof(int64_t));
    }

    std::string getString(size_t index) const override {
        int64_t ts = data[index];
        std::time_t t = static_cast<std::time_t>(ts);
        std::tm tm = *std::gmtime(&t);
        std::ostringstream oss;
        oss << "\"";
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        oss << "\"";
        return oss.str();
    }
};

using Int8Column   = TypedColumn<int8_t,   DataType::Int8>;
using Int16Column  = TypedColumn<int16_t,  DataType::Int16>;
using Int32Column  = TypedColumn<int32_t,  DataType::Int32>;
using Int64Column  = TypedColumn<int64_t,  DataType::Int64>;

std::shared_ptr<ColumnData> createColumn(DataType type);
class RowGroup {
public:
    explicit RowGroup(const Schema& schema, size_t maxUncompressedBytes = 128 * 1024 * 1024);

    void addRow(const std::vector<std::string>& row);
    bool isFull() const;
    void clear();
    void serialize(std::ostream& out) const;
    void deserialize(std::istream& in);

    size_t rowCount() const { return m_rows; }
    const std::vector<std::shared_ptr<ColumnData>>& getColumns() const { return m_columns; }

private:
    Schema m_schema;
    std::vector<std::shared_ptr<ColumnData>> m_columns;
    size_t m_maxBytes;
    size_t m_currentBytes;
    size_t m_rows;
};

} // namespace columnar