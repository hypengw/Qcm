#pragma once

#include <string_view>
#include <string>
#include <vector>

namespace qcm::model
{

struct ModelSql {
    std::vector<std::string_view> names;
    std::vector<int>              idxs;
    std::string                   select;
};

} // namespace qcm::model