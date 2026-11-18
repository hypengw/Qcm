export module qcm.backend:provider.lua.context;
import :provider.lua.support;
import :provider.run;
import luato;
import rstd;
using alloc::rc::Rc;

export namespace qcm::backend::bindings
{
struct LuaContext {
    rstd::sync::Weak<BindingsContext> owner;
    Rc<LuaCallState>                  call;
    auto clone() const -> LuaContext { return { owner.clone(), call.clone() }; }
};

auto install_context(BindingsContextArc context, luato::State& state) -> luato::Result<rstd::empty>;
} // namespace qcm::backend::bindings
