#pragma once

#include <filesystem>
#include <span>

namespace qcm
{

std::filesystem::path config_path();
std::filesystem::path data_path();
std::filesystem::path cache_path();

bool init_path(std::span<const std::filesystem::path>);

} // namespace qcm
