#pragma once
#include "batch.hpp"

#include <optional>

namespace columnar {

class IOperator {
public:
    virtual ~IOperator() = default;
    virtual std::optional<Batch> Next() = 0;
};

} // namespace columnar