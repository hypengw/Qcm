#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <ranges>

#include "core/fmt.h"

namespace qcm::model
{

struct ModelSql {
    struct Column {
        std::string_view name;
        std::string_view table;

        operator std::string() const { return fmt::format("{}.{}", table, name); }
    };
    static_assert(! std::convertible_to<Column, std::string_view>);

    std::vector<Column> columns;
    std::vector<int>    idxs;
    std::string         select;
    std::string         group_select;
};

} // namespace qcm::model