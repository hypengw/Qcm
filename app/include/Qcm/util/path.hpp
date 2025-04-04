#pragma once

#include <filesystem>
#include <span>

#include "core/core.h"

namespace qcm
{
auto config_path() -> std::filesystem::path;
auto data_path() -> std::filesystem::path;
auto cache_path() -> std::filesystem::path;

bool init_path(std::span<const std::filesystem::path>);
} // namespace qcm
