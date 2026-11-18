module;
#include <rstd/macro.hpp>
module qcm.backend;
import :provider.lua.session;
import :provider.lua_error;
import :uuid;
import :network.cookie_codec;
using rstd::async::coro;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend::bindings
{
namespace
{
auto inner_settings(BindingsContextArc context, luato::CallFrame& frame) -> luato::BindingResult {
    if (context->json.is_none()) return Err(fail("JSON utilities are unavailable"_str));
    return luato::return_value(
        frame,
        luato::to_value(frame,
                        context->common->settings().values,
                        luato::SerializeOptions::lua((*context->json)->deserialize_options())));
}
auto session_save_run(BindingsContextArc context, u64 generation, luato::FunctionHandle save)
    -> coro<luato::Result<String>> {
    auto write = co_await context->session->lock_save();
    if (! context->session->active() || context->generation.current() != generation)
        co_return Err(invalid_session());
    auto check  = context->generation.snapshot();
    auto result = rstd_co_try(save.call_values(luato::MultiValue::make()));
    auto values = rstd::move(result).into_values();
    if (values.len() != usize(1) || ! values[usize()].is_Bytes())
        co_return Err(fail("provider save must return a string"_str));
    auto custom =
        rstd_co_try(values[usize()].as_utf8(), fail("provider save must return UTF-8"_str));
    if (! context->session->active() || context->generation.current() != generation)
        co_return Err(invalid_session());
    Option<CookieSession> cookie_session;
    Option<CookieStamp>   cookie_stamp;
    if (context->http.is_some()) {
        cookie_session = Some((*context->http)->cookie_session());
        cookie_stamp   = Some(rstd_co_try(cookie_session->stamp(), http_error));
    }
    if (context->session->is_saved(custom, cookie_stamp, check)) co_return Ok("saved"_Str);
    auto cookies = String::make("[]"_str);
    if (cookie_session.is_some()) {
        auto encoded = rstd_co_try(encode_cookie_snapshot(*cookie_session), http_error);
        cookies      = rstd::move(encoded.text);
        cookie_stamp = Some(rstd::move(encoded.stamp));
    }
    auto stored = rstd_co_try(
        co_await context->session->save(
            String::make(custom), rstd::move(cookies), rstd::move(cookie_stamp), rstd::move(check)),
        [](ProviderError error) {
            return provider_error_to_lua(rstd::move(error), "qcm.inner.save_session"_str);
        });
    if (stored == SessionSaveResult::Invalid) co_return Err(invalid_session());
    if (! context->session->active() || context->generation.current() != generation)
        co_return Err(invalid_session());
    auto status = stored == SessionSaveResult::Saved ? "saved"_str : "staged"_str;
    co_return Ok(String::make(status));
}
auto session_task_run(BindingsContextArc context, u64 generation, SessionFlightLease lease,
                      luato::FunctionHandle callback) -> coro<luato::Result<luato::LuaValue>> {
    if (lease.owner()) {
        auto values = co_await callback.call_async_nested(luato::MultiValue::make());
        if (values.is_err()) {
            lease.flight().complete(Err(rstd::move(values).unwrap_err()));
        } else {
            auto output = rstd::move(*values).into_values();
            if (output.is_empty())
                lease.flight().complete(Ok(luato::LuaValue::Nil()));
            else
                lease.flight().complete(Ok(rstd::move(output[usize()])));
        }
    } else {
        (void)co_await lease.flight().wait();
    }
    auto result = rstd_co_try(lease.flight().result());
    if (! context->session->active() || context->generation.current() != generation)
        co_return Err(invalid_session());
    co_return Ok(rstd::move(result));
}
auto make_uuid() -> Result<String, luato::Error> {
    return random_uuid().map_err([](auto error) {
        return provider_error_to_lua(ProviderError::io(rstd::move(error)));
    });
}
auto begin_session(BindingsContextArc context) -> luato::Result<empty> {
    if (! context->session->active()) return Err(invalid_session());
    if (context->generation.current() >= u64(2305843009213693951ULL))
        return Err(fail("session generation exhausted"_str));
    if (context->http.is_some()) {
        auto reset = (*context->http)->reset_session();
        if (reset.is_err()) return Err(http_error(rstd::move(reset).unwrap_err()));
    }
    context->session->flights().cancel();
    if (! context->generation.advance()) return Err(fail("session generation exhausted"_str));
    context->session->reset_saved();
    return Ok(empty {});
}

struct LuaInner {
    BindingsContextArc context;
};
auto inner_context(const luato::UserDataHandle& self) -> luato::Result<BindingsContextArc> {
    return self.with<LuaInner>([](const LuaInner& inner) -> luato::Result<BindingsContextArc> {
        return Ok(inner.context.clone());
    });
}
} // namespace

auto install_inner(BindingsContextArc context, luato::State& state)
    -> luato::Result<luato::UserDataHandle> {
    auto inner = luato::UserDataSpec<LuaInner>("qcm.inner"_Str);
    inner.field_get("id"_Str, [](const LuaInner& inner) {
        return inner.context->common->id();
    });
    inner.field_get("name"_Str, [](const LuaInner& inner) {
        return String::make(inner.context->common->name());
    });
    inner.field_get("base_url"_Str, [](const LuaInner& inner) {
        return String::make(inner.context->common->base_url());
    });
    inner.field_get("type_name"_Str, [](const LuaInner& inner) {
        return String::make(inner.context->common->type_name());
    });
    inner.field_get("app_name"_Str, [](const LuaInner&) {
        return "Qcm"_Str;
    });
    inner.field_get("app_version"_Str, [](const LuaInner&) {
        return "0.1.0"_Str;
    });
    inner.raw_field_get("auth_method"_Str, [](luato::CallFrame& frame) -> luato::BindingResult {
        auto self    = rstd_try(luato::argument<luato::UserDataHandle>(frame, usize()));
        auto context = rstd_try(inner_context(self));
        if (context->json.is_none()) return Err(fail("JSON utilities are unavailable"_str));
        return luato::return_value(
            frame,
            luato::to_value(frame,
                            context->common->auth_method(),
                            luato::SerializeOptions::lua((*context->json)->deserialize_options())));
    });
    inner.method("device_id"_Str, [](const LuaInner& inner) {
        auto id = inner.context->common->device_id();
        return String::make(id.is_empty() ? "qcm-cpp-device"_str : id);
    });
    inner.method("new_session_id"_Str, [](const LuaInner&) {
        return make_uuid();
    });
    inner.method("begin_session"_Str, [](const LuaInner& inner) -> luato::Result<luato::LuaValue> {
        rstd_try(begin_session(inner.context.clone()));
        return Ok(luato::LuaValue::Nil());
    });
    inner.async_method("session_task"_Str,
                       [](luato::UserDataHandle self,
                          String                key,
                          luato::FunctionHandle callback) -> coro<luato::Result<luato::LuaValue>> {
                           auto context = rstd_co_try(inner_context(self));
                           if (! context->session->active()) co_return Err(invalid_session());
                           auto lease = context->session->flights().acquire(key.as_str());
                           co_return co_await session_task_run(context.clone(),
                                                               context->generation.current(),
                                                               rstd::move(lease),
                                                               rstd::move(callback));
                       });
    inner.async_method("save_session"_Str,
                       [](luato::UserDataHandle self) -> coro<luato::Result<String>> {
                           auto context = rstd_co_try(inner_context(self));
                           if (! context->session->active()) co_return Err(invalid_session());
                           if (context->session_save.is_none())
                               co_return Err(fail("provider save is missing"_str));
                           auto save = rstd_co_try(context->session_save->clone());
                           co_return co_await session_save_run(
                               context.clone(), context->generation.current(), rstd::move(save));
                       });
    inner.raw_method("settings"_Str, [](luato::CallFrame& frame) -> luato::BindingResult {
        auto self = rstd_try(luato::argument<luato::UserDataHandle>(frame, usize()));
        return inner_settings(rstd_try(inner_context(self)), frame);
    });
    rstd_try(state.register_userdata(rstd::move(inner)));
    return state.create_userdata(LuaInner { rstd::move(context) });
}
} // namespace qcm::backend::bindings
