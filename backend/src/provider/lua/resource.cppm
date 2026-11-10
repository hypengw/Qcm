module;
#include <rstd/enum.hpp>
#include <rstd/macro.hpp>
export module qcm.backend:provider.lua.resource;
export import :provider.lua.support;
import :provider.lua_error;
using qcm::backend::resource_cache::CacheMode;
using rstd::sync::Arc;
using rstd::sync::Weak;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
namespace bindings
{
struct PreparedResourceCall {
    BindingsContext::PreparedResource resource;
    Option<luato::FunctionHandle>     validator;
    Weak<BindingsContext>             owner;

    auto clone() const -> Result<PreparedResourceCall, luato::Error> {
        auto context = owner.upgrade();
        if (! context || ! context->session->active() ||
            context->generation.current() != resource.generation)
            return Err(invalid_session());
        Option<luato::FunctionHandle> copied;
        if (validator.is_some()) copied = Some(rstd_try(validator->clone()));
        return Ok(PreparedResourceCall { resource.clone(), rstd::move(copied), owner.clone() });
    }
};
struct PreparedResourceHandle {
    Weak<BindingsContext>        owner;
    Option<PreparedResourceCall> call;
};
inline auto take_resource_request(BindingsContext& context, const luato::UserDataHandle& input)
    -> Result<BindingsContext::PreparedResource, luato::Error> {
    namespace http = luato::extra::http;
    Option<resource_cache::PreparedRequest> prepared;
    (void)rstd_try(
        http::take_request(input, [&](const http::RequestData& request) -> luato::Result<empty> {
            prepared = Some(rstd_try(resource_cache::PreparedRequest::make(request.method.as_str(),
                                                                           request.url.as_str(),
                                                                           request.headers.clone(),
                                                                           request.body.is_some(),
                                                                           request.timeout),
                                     [](ProviderError error) {
                                         return provider_error_to_lua(rstd::move(error),
                                                                      "qcm.resource"_str);
                                     }));
            return Ok(empty {});
        }));
    return Ok(
        BindingsContext::PreparedResource { rstd::move(*prepared), context.generation.current() });
}
inline auto take_prepared(BindingsContextArc context, const luato::UserDataHandle& handle)
    -> Result<PreparedResourceCall, luato::Error> {
    if (! rstd_try(handle.is<PreparedResourceHandle>()))
        return Ok(PreparedResourceCall {
            rstd_try(take_resource_request(*context, handle)), None(), context.downgrade() });
    return handle.with_mut<PreparedResourceHandle>(
        [&](PreparedResourceHandle& value) -> Result<PreparedResourceCall, luato::Error> {
            auto owner = value.owner.upgrade();
            if (! owner || ! Arc<BindingsContext>::ptr_eq(owner, context))
                return Err(fail("resource belongs to another provider"_str));
            if (value.call.is_none()) return Err(fail("resource already consumed"_str));
            if (! context->session->active() ||
                value.call->resource.generation != context->generation.current())
                return Err(invalid_session());
            return Ok(rstd::move(*value.call.take()));
        });
}
inline auto resource_prepare(BindingsContextArc context, luato::CallFrame& frame)
    -> luato::BindingResult {
    auto count = frame.argument_count();
    if (count != usize(2) && count != usize(3))
        return Err(fail("qcm.resource.prepare expects a request, validator and options"_str));
    auto input = luato::extra::http::userdata_arg(frame);
    if (input.is_err()) return Err(rstd::move(input).unwrap_err());
    auto validate = take(frame, usize(1));
    if (validate.is_err() || ! validate->is_Function())
        return Err(fail("qcm.resource.prepare validator must be a function"_str));
    auto        cache_mode = CacheMode::ByteRange;
    Vec<String> transient_query;
    if (count == usize(3)) {
        auto options = take(frame, usize(2));
        if (options.is_err()) return Err(rstd::move(options).unwrap_err());
        if (! options->is_Nil() && ! options->is_Table())
            return Err(fail("qcm.resource.prepare options must be a table"_str));
        if (! options->is_Nil()) {
            auto table = rstd::move(*options).as_Table().value;
            auto keys  = table.keys();
            if (keys.is_err()) return Err(fail("qcm.resource.prepare options must be a table"_str));
            for (const auto& key : keys->as_slice()) {
                if (! key.is_Bytes()) return Err(fail("unknown resource option"_str));
                auto name = rstd::str_::from_utf8(key.as_Bytes().value.as_slice());
                if (name.is_err() || (*name != "cache_mode"_str && *name != "transient_query"_str))
                    return Err(fail("unknown resource option"_str));
                if (*name == "cache_mode"_str) {
                    auto value = table.get_in_call(key);
                    if (value.is_err() || ! value->is_Bytes())
                        return Err(fail("invalid resource cache mode"_str));
                    auto mode = rstd::str_::from_utf8(value->as_Bytes().value.as_slice());
                    if (mode.is_err()) return Err(fail("invalid resource cache mode"_str));
                    if (*mode == "byte_range"_str)
                        cache_mode = CacheMode::ByteRange;
                    else if (*mode == "whole_object"_str)
                        cache_mode = CacheMode::WholeObject;
                    else if (*mode == "document"_str)
                        cache_mode = CacheMode::Document;
                    else
                        return Err(fail("invalid resource cache mode"_str));
                } else {
                    auto value      = rstd_try(table.get_in_call(key));
                    transient_query = rstd_try(luato::from_value<Vec<String>>(value));
                }
            }
        }
    }
    auto resource = rstd_try(take_resource_request(*context, *input));
    resource.request =
        rstd_try(rstd::move(resource.request)
                     .with_cache_mode(cache_mode)
                     .with_transient_query(rstd::move(transient_query)),
                 [](ProviderError error) {
                     return provider_error_to_lua(rstd::move(error), "qcm.resource"_str);
                 });
    auto validator = rstd_try(validate->as_Function().value.clone());
    auto handle    = rstd_try(frame.create_userdata(PreparedResourceHandle {
        context.downgrade(),
        Some(PreparedResourceCall {
            rstd::move(resource), Some(rstd::move(validator)), context.downgrade() }) }));
    return luato::extra::http::return_value(frame, luato::LuaValue::UserData(rstd::move(handle)));
}

inline auto install_resource(BindingsContextArc context, luato::State& state)
    -> Result<luato::ModuleSpec, luato::Error> {
    rstd_try(
        state.register_userdata(luato::UserDataSpec<PreparedResourceHandle>("qcm.resource"_Str)));
    auto module = luato::ModuleSpec("qcm.resource"_Str);
    module.raw_variadic_function(
        "prepare"_Str,
        [context = context.clone()](luato::CallFrame& frame) -> luato::BindingResult {
            return resource_prepare(context.clone(), frame);
        });
    return Ok(rstd::move(module));
}

} // namespace bindings
} // namespace qcm::backend
