module;
#include <rstd/enum.hpp>
#include <rstd/macro.hpp>
export module qcm.backend:provider.lua_bindings;
export import :provider.lua.context;
export import :provider.lua.session;
export import :provider.lua.resource;
export import :provider.lua.installation;
import :provider.lua.enums;
import rstd.log;
using alloc::rc::Rc;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
namespace bindings
{
inline auto raise_provider_error(luato::CallFrame& frame) -> luato::BindingResult {
    if (frame.argument_count() == usize()) return Ok(usize());
    auto                       value = rstd_try(take(frame, usize()));
    Option<luato::TableHandle> input;
    RSTD_MATCH(value) {
        RSTD_CASE(Bytes, bytes) {
            auto text = rstd_try(value.as_utf8());
            return Err(provider_error_to_lua(ProviderError::Lua(String::make(text))));
        }
        RSTD_CASE(Table, table) {
            input = Some(rstd::move(table));
            break;
        }
        RSTD_CASE_DEFAULT { return Ok(usize()); }
    }
    auto& table    = *input;
    auto  existing = rstd_try(table.error_in_call());
    if (existing.is_some()) return Err(rstd::move(*existing));
    auto        code   = rstd_try(table.string_field_in_call("code"_str));
    auto        number = rstd_try(table.number_field_in_call("status"_str));
    Option<i32> status;
    if (number.is_some()) {
        auto integer = number->trunc();
        if (! integer.is_finite() || integer < f64(-2147483648.0) || integer > f64(2147483647.0))
            return Err(luato::Error::make(luato::ErrorKind::Type,
                                          String::make("qcm.error"_str),
                                          String::make("status is outside i32 range"_str)));
        status = Some(i32(static_cast<int>(integer.to_primitive())));
    }
    auto is_code = [&](ref<str> name) {
        return code.is_some() && *code == name;
    };
    auto is_status = [&](i32 expected) {
        return status.is_some() && *status == expected;
    };
    if (is_code("not_auth"_str) || is_status(i32(401))) {
        auto message = rstd_try(table.string_field_in_call("message"_str));
        auto jump    = rstd_try(table.string_field_in_call("jump_url"_str));
        if (jump.is_some()) {
            auto url = lihttpto::Url::parse_http(jump->as_str());
            if (url.is_err() || ! url->username().is_empty() ||
                (url->userinfo().is_some() && url->userinfo()->contains(":"_str)))
                jump = None();
        }
        if (message.is_some() || jump.is_some())
            return Err(provider_error_to_lua(ProviderError::AuthRequired(
                message.is_some() ? rstd::move(*message) : String::make("Login required"_str),
                rstd::move(jump))));
        return Err(provider_error_to_lua(ProviderError::NotAuth()));
    }
    if (is_code("not_found"_str) || is_status(i32(404)))
        return Err(provider_error_to_lua(ProviderError::NotFound()));
    if (is_code("not_implemented"_str))
        return Err(provider_error_to_lua(ProviderError::NotImplemented()));
    auto message = rstd_try(table.string_field_in_call("message"_str));
    return Err(provider_error_to_lua(
        ProviderError::Lua(rstd::format("{}: {}",
                                        code.is_some() ? code->as_str() : "error"_str,
                                        message.is_some() ? message->as_str() : ""_str))));
}
inline auto errors_inspect(luato::CallFrame& frame) -> luato::BindingResult {
    auto value = take(frame, usize());
    if (value.is_err()) return Err(rstd::move(value).unwrap_err());
    if (! value->is_Table()) {
        frame.push_nil();
        return Ok(usize(1));
    }
    auto source  = rstd::move(*value).as_Table().value;
    auto domain  = rstd_try(source.get_in_call(str_key("domain"_str)));
    auto code    = rstd_try(source.get_in_call(str_key("code"_str)));
    auto message = rstd_try(source.get_in_call(str_key("message"_str)));
    if (! domain.is_Bytes() && ! code.is_Bytes()) {
        frame.push_nil();
        return Ok(usize(1));
    }
    auto table = new_table(frame);
    if (table.is_err()) return Err(rstd::move(table).unwrap_err());
    auto handle = rstd::move(table).unwrap();
    rstd_try(handle.set_in_call(str_key("domain"_str), rstd::move(domain)));
    rstd_try(handle.set_in_call(str_key("code"_str), rstd::move(code)));
    rstd_try(handle.set_in_call(str_key("message"_str), rstd::move(message)));
    return_table(frame, rstd::move(handle));
    return Ok(usize());
}
inline auto debug_value(BindingsContextArc context, luato::CallFrame& frame)
    -> luato::BindingResult {
    auto value = take(frame, usize());
    if (value.is_err()) return Err(rstd::move(value).unwrap_err());
    if (context->json.is_none()) return Err(fail("JSON utilities are unavailable"_str));
    auto encoded = (*context->json)->encode(*value);
    if (encoded.is_err()) return Err(rstd::move(encoded).unwrap_err());
    rstd::log::info("{}", encoded->as_str());
    frame.push_nil();
    return Ok(usize(1));
}

inline auto install(BindingsContextArc context, luato::State& state)
    -> Result<empty, luato::Error> {
    rstd_try(install_context(context.clone(), state));
    auto resource = rstd_try(install_resource(context.clone(), state));
    auto qcm      = luato::ModuleSpec(String::make("qcm"_str));
    qcm.raw_variadic_function(String::make("error"_str),
                              [](luato::CallFrame& frame) -> luato::BindingResult {
                                  return raise_provider_error(frame);
                              });
    qcm.raw_function(String::make("get_http_client"_str),
                     usize(0),
                     [context = context.clone()](luato::CallFrame& frame) -> luato::BindingResult {
                         return (*context->http_bindings).get()->create_client(frame);
                     });
    qcm.raw_function(String::make("debug"_str),
                     usize(1),
                     [context = context.clone()](luato::CallFrame& frame) -> luato::BindingResult {
                         return debug_value(context.clone(), frame);
                     });
    auto errors = luato::ModuleSpec(String::make("qcm.errors"_str));
    errors.raw_function(
        String::make("inspect"_str), usize(1), [](luato::CallFrame& frame) -> luato::BindingResult {
            return errors_inspect(frame);
        });
    auto modules = Vec<luato::ModuleSpec>::make();
    modules.push(rstd::move(errors));
    modules.push(rstd::move(resource));
    modules.push(rstd::move(qcm));
    for (auto& module : modules) {
        auto registered = state.register_module(rstd::move(module));
        if (registered.is_err()) return Err(rstd::move(registered).unwrap_err());
    }
    auto utilities = rstd_try(luato::extra::install_utilities(state, "qcm"_str));
    context->json  = Some(utilities.json.clone());
    auto source    = Rc<luato::extra::http::ClientSource>(new HttpSource(context.downgrade()));
    context->http_bindings = Some(rstd_try(
        luato::extra::http::install(state, "qcm"_str, rstd::move(source), utilities.json.clone())));
    auto globals           = rstd_try(state.globals());
    auto qcm_table         = rstd_try(globals.get_table("qcm"_str));
    for (auto name : array<ref<str>, 3> { "resource"_str, "errors"_str, "http"_str }) {
        auto module_name = "qcm."_Str;
        module_name.push_str(name);
        auto value = rstd_try(globals.get_table(module_name.as_str()));
        rstd_try(qcm_table.set(str_key(name), luato::LuaValue::Table(rstd::move(value))));
    }
    auto inner = rstd_try(install_inner(context.clone(), state));
    rstd_try(qcm_table.set(str_key("inner"_str), luato::LuaValue::UserData(rstd::move(inner))));
    auto enums = rstd_try(install_enums(state));
    rstd_try(qcm_table.set(str_key("enum"_str), luato::LuaValue::Table(rstd::move(enums))));
    if (context->installation.is_some()) {
        auto installation = rstd_try(install_installation(context->installation->clone(), state));
        rstd_try(qcm_table.set(str_key("installation"_str),
                               luato::LuaValue::UserData(rstd::move(installation))));
    }
    return Ok(empty {});
}

} // namespace bindings
} // namespace qcm::backend
