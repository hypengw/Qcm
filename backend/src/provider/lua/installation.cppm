export module qcm.backend:provider.lua.installation;
export import :provider.lua.support;

export namespace qcm::backend::bindings
{
auto install_installation(PluginContext context, luato::State& state)
    -> luato::Result<luato::UserDataHandle>;
} // namespace qcm::backend::bindings
