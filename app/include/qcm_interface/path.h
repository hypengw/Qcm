#pragma once

#include <filesystem>
#include <span>

#include "qcm_interface/export.h"

namespace qcm
{
QCM_INTERFACE_API auto config_path() -> std::filesystem::path;
QCM_INTERFACE_API auto data_path() -> std::filesystem::path;
QCM_INTERFACE_API auto cache_path() -> std::filesystem::path;

QCM_INTERFACE_API bool init_path(std::span<const std::filesystem::path>);
} // namespace qcm
