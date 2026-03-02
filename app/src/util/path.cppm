module;
#include <filesystem>
#include <span>
export module qcm:util.path;
export import qcm.core;

export namespace qcm
{
auto config_path() -> std::filesystem::path;
auto data_path() -> std::filesystem::path;
auto cache_path() -> std::filesystem::path;

bool init_path(std::span<const std::filesystem::path>);
} // namespace qcm
