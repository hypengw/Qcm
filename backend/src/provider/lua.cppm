export module qcm.backend:provider.lua;
import :provider.factory;
import rstd;
using rstd::path::Path;
using namespace rstd::prelude;

export namespace qcm::backend
{
auto make_lua_provider(ref<Path> script, ProviderOptions options) -> ProviderResult<ProviderHandle>;
} // namespace qcm::backend
