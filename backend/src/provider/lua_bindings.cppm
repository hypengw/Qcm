export module qcm.backend:provider.lua_bindings;
import :provider.lua.support;
import luato;

export namespace qcm::backend::bindings
{
auto install(BindingsContextArc context, luato::State& state) -> luato::Result<rstd::empty>;
} // namespace qcm::backend::bindings
