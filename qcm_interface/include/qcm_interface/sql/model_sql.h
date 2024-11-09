#pragma once

#include <string_view>
#include <string>
#include <vector>

namespace qcm::model
{

struct ModelSql {
    std::vector<std::string_view> columns;
    std::vector<int>              idxs;
    std::string                   select;
    std::string                   group_select;
};

} // namespace qcm::model