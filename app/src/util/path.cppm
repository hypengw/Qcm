export module qcm:util.path;
export import qcm.core;

namespace cppstd = rstd::cppstd;
export namespace qcm
{
auto config_path() -> cppstd::filesystem::path;
auto data_path() -> cppstd::filesystem::path;
auto cache_path() -> cppstd::filesystem::path;

bool init_path(cppstd::span<const cppstd::filesystem::path>);
} // namespace qcm
