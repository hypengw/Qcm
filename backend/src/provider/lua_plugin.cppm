export module qcm.backend:provider.lua_plugin;
import :provider.registry;
import rstd;
using rstd::path::Path;
using namespace rstd::prelude;

export namespace qcm::backend
{
class LuaProviderPlugin {
public:
    static auto load_dir(ref<Path> dir) -> ProviderResult<ProviderRegistry>;
    static auto load_default() -> ProviderResult<ProviderRegistry>;
};
} // namespace qcm::backend
