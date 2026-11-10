module;
#include <rstd/enum.hpp>
#include <rstd/macro.hpp>
export module qcm.backend:provider.lua.installation;
export import :provider.lua.support;
using rstd::async::coro;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::bindings
{
inline auto installation_error(StateError error) -> luato::Error {
    if (! error.outcome_unknown) {
        RSTD_MATCH(error.reason) {
            RSTD_CASE(Storage, cause) {
                return provider_error_to_lua(ProviderError::db(rstd::move(cause)),
                                             "qcm.inner.state"_str);
            }
            RSTD_CASE_DEFAULT { break; }
        }
    }
    return provider_error_to_lua(ProviderError::external(rstd::move(error)), "qcm.inner.state"_str);
}
inline auto installation_state(luato::CallFrame& frame, InstallationState state)
    -> luato::Result<luato::TableHandle> {
    auto table = rstd_try(new_table(frame));
    rstd_try(table.set_in_call(str_key("revision"_str),
                               luato::LuaValue::Integer(i64(state.revision.to_primitive()))));
    if (state.data.is_some())
        rstd_try(table.set_in_call(str_key("data"_str),
                                   luato::LuaValue::Bytes(rstd::move(*state.data).into_bytes())));
    return Ok(rstd::move(table));
}
struct LuaInstallation {
    PluginContext context;
};
inline auto installation_context(const luato::UserDataHandle& self)
    -> luato::Result<PluginContext> {
    return self.with<LuaInstallation>(
        [](const LuaInstallation& object) -> luato::Result<PluginContext> {
            return Ok(object.context.clone());
        });
}
inline auto install_installation(PluginContext context, luato::State& state)
    -> luato::Result<luato::UserDataHandle> {
    auto object = luato::UserDataSpec<LuaInstallation>("qcm.installation"_Str);
    object.field_get("namespace"_Str, [](const LuaInstallation& object) {
        return String::make(object.context.namespace_name());
    });
    object.field_get("installation_id"_Str, [](const LuaInstallation& object) {
        return String::make(object.context.installation_id());
    });
    object.field_get("runtime_id"_Str, [](const LuaInstallation& object) {
        return String::make(object.context.runtime_id());
    });
    object.raw_field_get("platform"_Str, [](luato::CallFrame& frame) -> luato::BindingResult {
        auto        self    = rstd_try(luato::argument<luato::UserDataHandle>(frame, usize()));
        auto        context = rstd_try(installation_context(self));
        const auto& info    = context.platform();
        auto        table   = rstd_try(new_table(frame));
        rstd_try(table.set_in_call(str_key("os"_str), str_value(info.os)));
        rstd_try(table.set_in_call(str_key("arch"_str), str_value(info.arch)));
        rstd_try(table.set_in_call(str_key("family"_str), str_value(info.family)));
        rstd_try(table.set_in_call(str_key("source"_str), str_value(info.source)));
        return luato::return_value(frame, rstd::move(table));
    });
    object.async_method("load_state"_Str,
                        [](luato::UserDataHandle self) -> coro<luato::Result<luato::AsyncReturn>> {
                            auto context = rstd_co_try(installation_context(self));
                            auto loaded =
                                rstd_co_try(co_await context.load_state(), installation_error);
                            co_return Ok(luato::AsyncReturn::make(
                                [loaded = rstd::move(loaded)](
                                    luato::CallFrame& frame) mutable -> luato::BindingResult {
                                    return luato::return_value(
                                        frame, installation_state(frame, rstd::move(loaded)));
                                }));
                        });
    object.async_method(
        "compare_exchange_state"_Str,
        [](luato::UserDataHandle self,
           i64                   revision,
           String                data) -> coro<luato::Result<luato::AsyncReturn>> {
            if (revision < i64()) co_return Err(fail("state revision must be nonnegative"_str));
            auto context = rstd_co_try(installation_context(self));
            auto updated = rstd_co_try(co_await context.compare_exchange_state(
                                           u64(revision.to_primitive()), rstd::move(data)),
                                       installation_error);
            co_return Ok(luato::AsyncReturn::make(
                [updated =
                     rstd::move(updated)](luato::CallFrame& frame) mutable -> luato::BindingResult {
                    auto table = rstd_try(new_table(frame));
                    auto state = rstd_try(installation_state(frame, rstd::move(updated.state)));
                    rstd_try(table.set_in_call(str_key("applied"_str),
                                               luato::LuaValue::Boolean(updated.applied)));
                    rstd_try(table.set_in_call(str_key("state"_str),
                                               luato::LuaValue::Table(rstd::move(state))));
                    return luato::return_value(frame, rstd::move(table));
                }));
        });
    rstd_try(state.register_userdata(rstd::move(object)));
    return state.create_userdata(LuaInstallation { rstd::move(context) });
}
} // namespace qcm::backend::bindings
