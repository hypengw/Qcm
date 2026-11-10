module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:provider.lua;
import luato;
import :provider.http_bridge;
import :resource.prepared;
import :provider.common;
export import :provider.factory;
import :provider.lua_impl;
import rstd.json;
import :provider.lua_error;
import :provider.lua_bindings;
import :provider.batch;
import :model.home;
import :model.radio;
import licrypto;
import :provider.run;
import :playing;
import rstd;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using qcm::backend::resource_cache::PreparedRequest;
using qcm::backend::resource_cache::ResourceBody;
using rstd::async::RuntimeHandle;
using rstd::bytes::Bytes;
using rstd::path::Path;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::Weak;
using ResourceResponse = qcm::backend::resource_cache::Response;
using ResourceError    = qcm::backend::resource_cache::Error;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
template<class T, class E>
auto provider_result(Result<T, E> result) -> ProviderResult<T> {
    if constexpr (rstd::mtp::same_as<E, ProviderError>) {
        return result;
    } else {
        return rstd::move(result).map_err([](E error) {
            return rstd::into<ProviderError>(rstd::move(error));
        });
    }
}
template<class T>
auto provider_result(rstd::async::coro<Result<T, luato::Error>> task)
    -> rstd::async::coro<ProviderResult<T>> {
    co_return provider_result(co_await rstd::move(task));
}
} // namespace qcm::backend

namespace qcm::backend
{
struct LuaProviderOptions {
    String                type_name;
    String                name;
    String                base_url;
    SettingsSnapshot      settings;
    Option<AuthMethod>    auth_method;
    String                device_id;
    PluginConfig          config;
    Option<PluginContext> installation;
    Option<i64>           id;
    Arc<PlayingState>     playing { Arc<PlayingState>::make() };
};
class LuaProviderHost {
    luato::State                  state_;
    Arc<ProviderCommonData>       common_;
    Arc<BindingsContext>          bindings_;
    LuaImpl                       funcs_;
    Option<luato::FunctionHandle> resource_scope_;
    Vec<SettingField>             settings_definition_;
    String                        saved_session_;

    LuaProviderHost(luato::State state, Arc<BindingsContext> bindings, LuaImpl funcs,
                    Option<luato::FunctionHandle> resource_scope, Vec<SettingField> settings)
        : state_(rstd::move(state)),
          common_(bindings->common.clone()),
          bindings_(rstd::move(bindings)),
          funcs_(rstd::move(funcs)),
          resource_scope_(rstd::move(resource_scope)),
          settings_definition_(rstd::move(settings)) {}

public:
    static auto make(ref<Path> script, LuaProviderOptions options,
                     Option<Arc<LuaHttpBridge>> http = None())
        -> Result<LuaProviderHost, luato::Error> {
        rstd_try(options.config.validate(), provider_error_to_lua(rstd::move(_e)));
        if (options.installation.is_some() &&
            options.installation->namespace_name() != options.type_name.as_str())
            return Err(bindings::fail("plugin context namespace does not match provider type"_str));
        auto state = rstd_try(luato::State::create(luato::StateOptions::build_script()));
        auto common =
            Arc<ProviderCommonData>::make(ProviderCommonData { rstd::move(options.type_name),
                                                               rstd::move(options.name),
                                                               rstd::move(options.base_url),
                                                               rstd::move(options.settings),
                                                               rstd::move(options.auth_method),
                                                               rstd::move(options.device_id),
                                                               options.id });
        auto context          = Arc<BindingsContext>::make(common.clone());
        context->installation = rstd::move(options.installation);
        context->playing      = rstd::move(options.playing);
        context->http         = rstd::move(http);
        if (context->http.is_some()) (*context->http)->set_scope(common->id());
        auto script_path = PathBuf::from(script);
        auto parent      = script_path.as_path().parent();
        if (parent.is_some()) {
            auto base = String::make(parent->to_string_lossy().as_str());
            auto resolver =
                [base = rstd::move(base)](
                    luato::ModuleRequest request) -> Result<luato::LuaModuleSource, luato::Error> {
                auto path = String::make(base);
                path.push_str("/"_str);
                for (const auto& byte : request.requested.as_str().as_bytes()) {
                    auto value = byte.to_primitive();
                    path.push_ascii(value == '.' ? '/' : static_cast<char>(value));
                }
                path.push_str(".lua"_str);
                auto bytes = rstd::fs::read(path.as_str());
                if (bytes.is_err())
                    return Err(luato::Error::make(luato::ErrorKind::Module,
                                                  rstd::move(request.requested),
                                                  String::make("Lua module not found"_str))
                                   .with_cause(ProviderError::io(rstd::move(bytes).unwrap_err())));
                return Ok(luato::LuaModuleSource {
                    request.requested.clone(), path.clone(), path.clone(), rstd::move(*bytes) });
            };
            (void)rstd_try(
                state.set_module_resolver(luato::ModuleResolverSpec::make(rstd::move(resolver))));
        }
        (void)rstd_try(bindings::install(context.clone(), state));
        auto text = rstd::fs::read_to_string(script);
        if (text.is_err())
            return Err(luato::Error::make(luato::ErrorKind::Module,
                                          String::make("plugin:script"_str),
                                          String::make("cannot read provider script"_str))
                           .with_cause(ProviderError::io(rstd::move(text).unwrap_err())));
        auto identity = String::make("plugin:"_str);
        identity.push_str(common->type_name());
        auto source = luato::LuaModuleSource { String::make(common->type_name()),
                                               rstd::move(identity),
                                               String::make("provider.lua"_str),
                                               Vec<u8>::from(text->as_str().as_bytes()) };
        auto values = rstd_try(state.evaluate_entry(rstd::move(source)));
        if (values.len() != usize(1))
            return Err(
                luato::Error::make(luato::ErrorKind::Module,
                                   String::make("plugin:script"_str),
                                   String::make("provider script must return one table"_str)));
        auto entries = rstd::move(values).into_values();
        auto first   = rstd::move(entries[usize()]);
        if (! first.is_Table())
            return Err(
                luato::Error::make(luato::ErrorKind::Module,
                                   String::make("plugin:script"_str),
                                   String::make("provider script must return one table"_str)));
        auto module           = rstd::move(first).as_Table().value;
        auto resource_scope   = rstd_try(optional_lua_function(module, "resource_scope"_str));
        auto funcs            = rstd_try(LuaImpl::read(module));
        context->session_save = Some(rstd_try(funcs.save.clone()));
        auto settings         = rstd_try(optional_lua_function(module, "settings_definition"_str));
        auto host             = LuaProviderHost(rstd::move(state),
                                                rstd::move(context),
                                                rstd::move(funcs),
                                                rstd::move(resource_scope),
                                                {});
        if (settings.is_some()) {
            auto result = rstd_try(settings->call<luato::LuaValue>(single_result()));
            host.settings_definition_ =
                rstd_try(host.decode_result<Vec<SettingField>>(rstd::move(result)));
        }
        return Ok(rstd::move(host));
    }
    auto deserialize_options() const -> luato::DeserializeOptions {
        if (bindings_->json.is_some()) return (*bindings_->json)->deserialize_options();
        return {};
    }
    template<class T>
    auto encode(const T& value) -> Result<luato::LuaValue, luato::Error> {
        return luato::to_value(state_, value, luato::SerializeOptions::lua(deserialize_options()));
    }
    template<class T>
    auto decode_result(luato::LuaValue result) const -> Result<T, luato::Error> {
        return luato::from_value<T>(result, deserialize_options());
    }
    static auto single_result(ref<str> message = "provider must return one value"_str)
        -> luato::ReturnOptions {
        return { Some(bindings::fail(message)) };
    }
    auto open_playback_async(PlaybackRequest request)
        -> rstd::async::coro<Result<PreparedPlayback, luato::Error>> {
        if (funcs_.open_playback.is_none()) co_return Ok(PreparedPlayback::byte_resource());
        auto input  = rstd_co_try(encode(request));
        auto epoch  = lifecycle_epoch();
        auto result = rstd_co_try(co_await funcs_.open_playback->call_async_typed<luato::LuaValue>(
            single_result(), rstd::move(input)));
        auto prepared = rstd_co_try(decode_result<PreparedPlayback>(rstd::move(result)));
        if (lifecycle_epoch() != epoch)
            co_return Err(provider_error_to_lua(ProviderError::NotAuth()));
        co_return Ok(rstd::move(prepared));
    }
    auto session_epoch() const -> Option<u64> {
        if (! bindings_->session->active()) return None();
        return Some(bindings_->generation.current());
    }
    auto lifecycle_epoch() const -> u64 { return bindings_->generation.current(); }
    auto generation_check() const -> ProviderGenerationCheck {
        return bindings_->generation.snapshot();
    }
    auto status_snapshot() const -> ProviderCommonSnapshot { return common_->status_snapshot(); }
    auto base_url() const -> String { return String::make(common_->base_url()); }
    auto auth_method() const -> Option<AuthMethod> {
        return common_->auth_method().is_some() ? Some(common_->auth_method()->clone()) : None();
    }
    auto matches_session(i64 provider, ref<str> key) const -> bool {
        return bindings_->session->matches(provider, key);
    }
    auto session_key() const -> Option<String> {
        if (! bindings_->session->active()) return None();
        return Some(bindings_->session->key());
    }
    auto settings() const -> SettingsSnapshot { return common_->settings().clone(); }
    auto set_name(ref<str> name) -> Result<empty, luato::Error> {
        common_->set_name(name);
        return Ok(empty {});
    }
    auto set_identity(i64 id, ref<str> name) -> Result<empty, luato::Error> {
        common_->set_identity(id, name);
        if (bindings_->http.is_some()) (*bindings_->http)->set_scope(Some(id));
        return Ok(empty {});
    }
    auto set_auth_info(AuthInput input) -> Result<empty, luato::Error> {
        common_->set_auth_info(rstd::move(input));
        return Ok(empty {});
    }
    auto set_settings(SettingsSnapshot values) -> Result<empty, luato::Error> {
        common_->set_settings(rstd::move(values));
        return Ok(empty {});
    }
    auto validate_settings_value(const SettingsValues& values)
        -> Result<Option<SettingsValues>, luato::Error> {
        if (funcs_.validate_settings.is_none()) return Ok(None());
        if (bindings_->json.is_none())
            return Err(bindings::fail("JSON utilities are unavailable"_str));
        auto input   = rstd_try(encode(values));
        auto result  = rstd_try(funcs_.validate_settings->call<luato::LuaValue>(
            single_result("invalid settings result"_str), rstd::move(input)));
        auto changed = rstd_try(decode_result<rstd::json::Map>(rstd::move(result)));
        return Ok(Some(rstd::move(changed)));
    }
    struct ResourceStreamState {
        luato::extra::http::BodyHandle body;
        auto next() -> rstd::async::coro<Result<Option<Bytes>, ResourceError>> {
            if (body->check().is_err()) {
                body->cancel();
                co_return Err(ResourceError::invalidated());
            }
            auto result = co_await body->next();
            if (body->check().is_err()) {
                body->cancel();
                co_return Err(ResourceError::invalidated());
            }
            co_return Ok(rstd_co_try(rstd::move(result), [](luato::Error error) {
                return ResourceError::provider(rstd::into<ProviderError>(rstd::move(error)));
            }));
        }
        auto cancel() -> void { body->cancel(); }
        auto close() -> rstd::async::coro<empty> { return body->close(); }
    };
    static auto resource_payload(luato::extra::http::ResponseData response) -> ResourceResponse {
        ResourceResponse payload;
        payload.status = i32(response.status.to_primitive());
        if (! response.url.is_empty()) payload.effective_url = Some(rstd::move(response.url));
        payload.body    = ResourceBody::from_bytes(Bytes::from_vec(rstd::move(response.body)));
        payload.headers = rstd::move(response.headers);
        return payload;
    }
    auto execute_prepared_async(bindings::PreparedResourceCall call, bool stream_response)
        -> rstd::async::coro<ProviderResult<ResourceResponse>> {
        namespace http = luato::extra::http;
        auto owner     = call.owner.upgrade();
        if (! owner || ! Arc<BindingsContext>::ptr_eq(owner, bindings_))
            co_return Err(ProviderError::internal("resource belongs to another provider"_str));
        if (bindings_->http.is_none())
            co_return Err(
                rstd::into<ProviderError>(bindings::fail("http bridge is unavailable"_str)));
        auto generation = call.resource.generation;
        auto current    = [&]() {
            return bindings_->session->active() && bindings_->generation.current() == generation;
        };
        if (! current()) co_return Err(ProviderError::NotAuth());
        auto                     parsed = rstd::move(call.resource.request);
        http::ResponseData       response;
        Option<http::BodyHandle> body;
        auto                     session = (*bindings_->http)->cookie_session();
        if (stream_response) {
            auto stream =
                rstd_co_try(co_await (*bindings_->http)
                                ->open_prepared_async(
                                    parsed.request_parts(), parsed.timeout(), session.clone()));
            if (! current()) {
                stream.cancel();
                (void)co_await stream.close();
                co_return Err(ProviderError::NotAuth());
            }
            response.status  = stream.head().status.value();
            response.headers = stream.head().headers.clone();
            response.url     = String::make(stream.url());
            body =
                Some(http::BodyHandle::make(lua_http::Body { rstd::move(stream),
                                                             session.clone(),
                                                             bindings_->http->clone(),
                                                             bindings_->generation.snapshot() }));
        } else {
            auto result = co_await (*bindings_->http)
                              ->request_prepared_async(parsed.request_parts(),
                                                       usize(8 * 1024 * 1024),
                                                       parsed.timeout(),
                                                       session.clone());
            if (result.is_err()) {
                rstd::log::warn("resource request failed");
                co_return Err(ProviderError::request(rstd::move(result).unwrap_err()));
            }
            response = lua_http::from_network(rstd::move(result).unwrap());
        }
        if (! current()) co_return Err(ProviderError::NotAuth());
        if (call.validator.is_some()) {
            auto handle =
                rstd_co_try(http::create_response(state_, rstd::move(response), rstd::move(body)));
            auto native = rstd_co_try(http::response(handle));
            struct Guard {
                Rc<http::Response> value;
                ~Guard() { value.get()->expire(); }
            } guard { native.clone() };
            auto accepted =
                co_await call.validator->call_async_typed<luato::IgnoreReturns>(rstd::move(handle));
            if (accepted.is_err()) {
                (void)native.get()->close();
                co_return Err(rstd::into<ProviderError>(rstd::move(accepted).unwrap_err()));
            }
            auto parts = rstd_co_try(native.get()->release());
            response   = rstd::move(parts.data);
            body       = rstd::move(parts.body);
        }
        if (! current()) {
            if (body.is_some()) (*body)->cancel();
            co_return Err(ProviderError::NotAuth());
        }
        auto payload = resource_payload(rstd::move(response));
        if (body.is_some()) {
            auto state = Arc<ResourceStreamState>::make(ResourceStreamState { rstd::move(*body) });
            payload.body = resource_cache::resource_body(rstd::move(state));
        }
        co_return Ok(rstd::move(payload));
    }

private:
    template<class... Args>
    auto prepare_resource_call_async(luato::FunctionHandle    function,
                                     const lihttpto::Headers& extra_headers, Args... arguments)
        -> rstd::async::coro<Result<bindings::PreparedResourceCall, luato::Error>> {
        auto generation = session_epoch();
        if (generation.is_none())
            co_return Err(provider_error_to_lua(ProviderError::NotAuth(), "provider:session"_str));
        auto result = rstd_co_try(
            co_await function.call_async_typed<luato::MultiValue>(rstd::move(arguments)...));
        auto current = session_epoch();
        if (current.is_none() || *current != *generation)
            co_return Err(provider_error_to_lua(ProviderError::NotAuth(), "provider:session"_str));
        // Stale sessions take precedence over invalid resource return values.
        auto handle = rstd_co_try(luato::from_multi<luato::UserDataHandle>(
            rstd::move(result),
            luato::ReturnOptions {
                Some(luato::Error::make(luato::ErrorKind::Module,
                                        "provider:resource"_Str,
                                        "resource must be a prepared request"_Str)) }));
        auto call   = rstd_co_try(bindings::take_prepared(bindings_.clone(), handle));
        rstd_co_try(call.resource.request.apply_headers(extra_headers), [](ProviderError error) {
            return provider_error_to_lua(rstd::move(error), "provider:resource"_str);
        });
        co_return Ok(rstd::move(call));
    }

public:
    auto resource_scope() -> Result<Option<String>, luato::Error> {
        if (resource_scope_.is_none()) return Ok(None());
        auto values = rstd_try(resource_scope_->call<luato::MultiValue>());
        if (values.is_empty())
            return Err(bindings::fail("resource_scope must return a string"_str));
        return Ok(Some(String::make(rstd_try(values.values()[usize()].as_utf8()))));
    }
    auto prepare_playback_call_async(ref<str> state, ref<str> uri, const lihttpto::Headers& headers)
        -> rstd::async::coro<Result<bindings::PreparedResourceCall, luato::Error>> {
        if (funcs_.prepare_playback_resource.is_none())
            co_return Err(
                provider_error_to_lua(ProviderError::NotImplemented(), "provider:resource"_str));
        co_return co_await prepare_resource_call_async(
            rstd_co_try(funcs_.prepare_playback_resource->clone()),
            headers,
            String::make(state),
            String::make(uri));
    }
    auto keep_playback_async(String state) -> rstd::async::coro<Result<empty, luato::Error>> {
        if (funcs_.keep_playback.is_some())
            (void)rstd_co_try(co_await funcs_.keep_playback->call_async_typed<luato::IgnoreReturns>(
                rstd::move(state)));
        co_return Ok(empty {});
    }
    auto close_playback_async(String state) -> rstd::async::coro<Result<empty, luato::Error>> {
        if (funcs_.close_playback.is_some())
            (void)rstd_co_try(
                co_await funcs_.close_playback->call_async_typed<luato::IgnoreReturns>(
                    rstd::move(state)));
        co_return Ok(empty {});
    }
    auto prepare_resource_async(Resource resource, const lihttpto::Headers& headers)
        -> rstd::async::coro<Result<bindings::PreparedResourceCall, luato::Error>> {
        RSTD_MATCH(resource) {
            RSTD_CASE(Image, item_id, image_id, kind) {
                if (funcs_.prepare_image.is_none())
                    co_return Err(provider_error_to_lua(ProviderError::NotImplemented(),
                                                        "provider:resource"_str));
                auto name = rstd::format("{}", kind);
                co_return co_await prepare_resource_call_async(
                    rstd_co_try(funcs_.prepare_image->clone()),
                    headers,
                    rstd::move(item_id),
                    rstd::move(image_id),
                    rstd::move(name));
            }
            RSTD_CASE(Audio, item_id) {
                if (funcs_.prepare_audio.is_none())
                    co_return Err(provider_error_to_lua(ProviderError::NotImplemented(),
                                                        "provider:resource"_str));
                co_return co_await prepare_resource_call_async(
                    rstd_co_try(funcs_.prepare_audio->clone()), headers, rstd::move(item_id));
            }
        }
    }

private:
    template<class T>
    auto home_async(luato::FunctionHandle function, Rc<LuaCallState> call, Option<String> block_id,
                    i32 page, i32 page_size) -> rstd::async::coro<Result<T, luato::Error>> {
        auto context = rstd_co_try(co_await run_context_async(rstd::move(call)));
        auto task =
            block_id.is_some()
                ? function.call_async_typed<luato::LuaValue>(single_result(),
                                                             rstd::move(context),
                                                             rstd::move(*block_id),
                                                             i64(page.to_primitive()),
                                                             i64(page_size.to_primitive()))
                : function.call_async_typed<luato::LuaValue>(single_result(), rstd::move(context));
        auto result = rstd_co_try(co_await rstd::move(task));
        co_return decode_result<T>(rstd::move(result));
    }

public:
    auto home_blocks_async(Rc<LuaCallState> call)
        -> rstd::async::coro<Result<Vec<HomeBlock>, luato::Error>> {
        if (funcs_.home_blocks.is_none())
            co_return Err(
                provider_error_to_lua(ProviderError::NotImplemented(), "provider:home"_str));
        auto value = rstd_co_try(co_await home_async<Vec<HomeBlock>>(
            rstd_co_try(funcs_.home_blocks->clone()), rstd::move(call), None(), i32(), i32()));
        co_return Ok(rstd::move(value));
    }
    auto home_block_items_async(Rc<LuaCallState> call, String block_id, i32 page, i32 page_size)
        -> rstd::async::coro<Result<HomeBlockItems, luato::Error>> {
        if (funcs_.home_block_items.is_none())
            co_return Err(
                provider_error_to_lua(ProviderError::NotImplemented(), "provider:home"_str));
        auto value = rstd_co_try(
            co_await home_async<HomeBlockItems>(rstd_co_try(funcs_.home_block_items->clone()),
                                                rstd::move(call),
                                                Some(rstd::move(block_id)),
                                                page,
                                                page_size));
        co_return Ok(rstd::move(value));
    }
    auto for_you_items_async(Rc<LuaCallState> call)
        -> rstd::async::coro<Result<Vec<HomeEntry>, luato::Error>> {
        if (funcs_.for_you_items.is_none())
            co_return Err(provider_error_to_lua(ProviderError::NotImplemented()));
        auto context = rstd_co_try(co_await run_context_async(rstd::move(call)));
        auto called  = rstd_co_try(co_await funcs_.for_you_items->call_async_typed<luato::LuaValue>(
            single_result(), rstd::move(context)));
        co_return decode_result<Vec<HomeEntry>>(rstd::move(called));
    }
    auto comments_async(Rc<LuaCallState> call, provider_comment::CommentTarget target,
                        Option<String> parent, provider_comment::CommentPageRequest request)
        -> rstd::async::coro<Result<Option<provider_comment::CommentPage>, luato::Error>> {
        auto& function = parent.is_some() ? funcs_.comment_replies : funcs_.comments;
        if (function.is_none()) co_return Ok(None());
        auto context       = rstd_co_try(co_await run_context_async(rstd::move(call)));
        auto target_value  = rstd_co_try(encode(target));
        auto request_value = rstd_co_try(encode(request));
        auto task = parent.is_some()
                        ? function->call_async_typed<luato::MultiValue>(rstd::move(context),
                                                                        rstd::move(target_value),
                                                                        rstd::move(*parent),
                                                                        rstd::move(request_value))
                        : function->call_async_typed<luato::MultiValue>(rstd::move(context),
                                                                        rstd::move(target_value),
                                                                        rstd::move(request_value));
        auto result = rstd_co_try(co_await rstd::move(task));
        auto values = rstd::move(result).into_values();
        if (values.is_empty()) co_return Ok(None());
        co_return decode_result<Option<provider_comment::CommentPage>>(rstd::move(values[usize()]));
    }
    auto set_comment_like_async(Rc<LuaCallState> call, provider_comment::CommentTarget target,
                                String comment_id, bool value)
        -> rstd::async::coro<Result<provider_comment::CommentLikeResult, luato::Error>> {
        if (funcs_.set_comment_like.is_none())
            co_return Err(
                provider_error_to_lua(ProviderError::NotImplemented(), "provider:comments"_str));
        auto context      = rstd_co_try(co_await run_context_async(rstd::move(call)));
        auto target_value = rstd_co_try(encode(target));
        auto result =
            rstd_co_try(co_await funcs_.set_comment_like->call_async_typed<luato::LuaValue>(
                single_result("comment like must be a table"_str),
                rstd::move(context),
                rstd::move(target_value),
                rstd::move(comment_id),
                value));
        if (! result.is_Table()) co_return Err(bindings::fail("comment like must be a table"_str));
        co_return decode_result<provider_comment::CommentLikeResult>(rstd::move(result));
    }
    auto qr_async() -> rstd::async::coro<Result<QrInfo, luato::Error>> {
        if (funcs_.qr.is_none())
            co_return Err(
                provider_error_to_lua(ProviderError::NotImplemented(), "provider:qr"_str));
        auto result =
            rstd_co_try(co_await funcs_.qr->call_async_typed<luato::LuaValue>(single_result()));
        co_return decode_result<QrInfo>(rstd::move(result));
    }
    auto share_url(ref<str> native_id, i64 item_type) -> Result<Option<String>, luato::Error> {
        if (funcs_.share_url.is_none()) return Ok(None());
        auto result = rstd_try(funcs_.share_url->call<luato::MultiValue>(native_id, item_type));
        auto values = rstd::move(result).into_values();
        if (values.len() != usize(1) || values[usize()].is_Nil()) return Ok(None());
        auto text = values[usize()].as_utf8();
        if (text.is_err())
            return Err(luato::Error::make(luato::ErrorKind::Type,
                                          String::make("provider:share"_str),
                                          String::make("share url must be text"_str)));
        return Ok(Some(String::make(*text)));
    }

private:
    template<class T>
    auto queue_call_async(luato::FunctionHandle function, Rc<LuaCallState> call, String native_id,
                          Option<luato::LuaValue> request = None())
        -> rstd::async::coro<Result<T, luato::Error>> {
        auto context = rstd_co_try(co_await run_context_async(rstd::move(call)));
        auto task    = request.is_some()
                           ? function.call_async_typed<luato::LuaValue>(single_result(),
                                                                        rstd::move(context),
                                                                        rstd::move(native_id),
                                                                        rstd::move(*request))
                           : function.call_async_typed<luato::LuaValue>(
                                 single_result(), rstd::move(context), rstd::move(native_id));
        auto result  = rstd_co_try(co_await rstd::move(task));
        co_return decode_result<T>(rstd::move(result));
    }
    static auto queue_not_implemented() -> luato::Error {
        return provider_error_to_lua(ProviderError::NotImplemented(), "provider:queue"_str);
    }

public:
    auto legacy_queue_async(Rc<LuaCallState> call, String native_id, Vec<String> current_ids,
                            QueueConfiguration configuration, Option<String> old_state)
        -> rstd::async::coro<Result<RadioBatch, luato::Error>> {
        if (funcs_.get_queue_next.is_none()) co_return Err(queue_not_implemented());
        auto context = rstd_co_try(co_await run_context_async(rstd::move(call)));
        auto ids =
            rstd_co_try(luato::to_value(state_, current_ids, luato::SerializeOptions::lua()));
        auto config =
            rstd_co_try(luato::to_value(state_, configuration, luato::SerializeOptions::lua()));
        if (old_state.is_none())
            old_state = Some(rstd_co_try(legacy_queue_state(configuration),
                                         provider_error_to_lua(rstd::move(_e))));
        auto result = rstd_co_try(
            co_await funcs_.get_queue_next->call_async_typed<luato::LuaValue>(single_result(),
                                                                              rstd::move(context),
                                                                              rstd::move(native_id),
                                                                              rstd::move(ids),
                                                                              rstd::move(config)));
        RadioBatch batch;
        batch.songs         = rstd_co_try(decode_result<Vec<Song>>(rstd::move(result)));
        batch.state         = rstd::move(*old_state);
        batch.configuration = rstd::move(configuration);
        co_return Ok(rstd::move(batch));
    }

    auto open_queue_async(Rc<LuaCallState> call, String native_id, QueueOpenRequest request)
        -> rstd::async::coro<Result<RadioBatch, luato::Error>> {
        if (funcs_.open_queue.is_none()) {
            if (request.input.is_some()) co_return Err(queue_not_implemented());
            co_return co_await legacy_queue_async(rstd::move(call),
                                                  rstd::move(native_id),
                                                  {},
                                                  rstd::move(request.configuration),
                                                  None());
        }
        auto encoded = rstd_co_try(
            co_await queue_call_async<RadioBatch>(rstd_co_try(funcs_.open_queue->clone()),
                                                  rstd::move(call),
                                                  rstd::move(native_id),
                                                  Some(rstd_co_try(encode(request)))));
        co_return Ok(rstd::move(encoded));
    }
    auto next_queue_async(Rc<LuaCallState> call, String native_id, QueueNextRequest request)
        -> rstd::async::coro<Result<RadioBatch, luato::Error>> {
        if (funcs_.next_queue.is_none()) {
            auto configuration = rstd_co_try(legacy_queue_configuration(request.state.as_str()),
                                             provider_error_to_lua(rstd::move(_e)));
            co_return co_await legacy_queue_async(rstd::move(call),
                                                  rstd::move(native_id),
                                                  rstd::move(request.current_native_ids),
                                                  rstd::move(configuration),
                                                  Some(rstd::move(request.state)));
        }
        auto encoded = rstd_co_try(
            co_await queue_call_async<RadioBatch>(rstd_co_try(funcs_.next_queue->clone()),
                                                  rstd::move(call),
                                                  rstd::move(native_id),
                                                  Some(rstd_co_try(encode(request)))));
        co_return Ok(rstd::move(encoded));
    }
    auto queue_definition_async(Rc<LuaCallState> call, String native_id)
        -> rstd::async::coro<Result<RadioDefinition, luato::Error>> {
        bool  legacy   = funcs_.queue_definition.is_none();
        auto& function = legacy ? funcs_.queue_configuration : funcs_.queue_definition;
        if (function.is_none()) co_return Ok(RadioDefinition {});
        if (legacy) {
            RadioDefinition definition;
            definition.configuration = rstd_co_try(co_await queue_call_async<Vec<RadioConfigField>>(
                rstd_co_try(function->clone()), rstd::move(call), rstd::move(native_id)));
            co_return Ok(rstd::move(definition));
        }
        co_return co_await queue_call_async<RadioDefinition>(
            rstd_co_try(function->clone()), rstd::move(call), rstd::move(native_id));
    }
    auto close_queue_async(Rc<LuaCallState> call, String native_id, String state)
        -> rstd::async::coro<Result<empty, luato::Error>> {
        if (funcs_.close_queue.is_none()) co_return Ok(empty {});
        auto context = rstd_co_try(co_await run_context_async(rstd::move(call)));
        (void)rstd_co_try(co_await funcs_.close_queue->call_async_typed<luato::IgnoreReturns>(
            rstd::move(context), rstd::move(native_id), rstd::move(state)));
        co_return Ok(empty {});
    }
    auto run_context_async(Rc<LuaCallState> call)
        -> rstd::async::coro<Result<luato::LuaValue, luato::Error>> {
        if (! bindings_->session->active())
            co_return Err(provider_error_to_lua(ProviderError::NotAuth(), "provider:context"_str));
        rstd_co_try(call.get()->bind_generation(bindings_->generation.snapshot()));
        auto context = rstd_co_try(state_.create_userdata(
            bindings::LuaContext { bindings_.downgrade(), rstd::move(call) }));
        co_return Ok(luato::LuaValue::UserData(rstd::move(context)));
    }
    auto sync_item_async(Rc<LuaCallState> call, Item item)
        -> rstd::async::coro<Result<empty, luato::Error>> {
        auto context = rstd_co_try(co_await run_context_async(rstd::move(call)));
        auto input   = rstd_co_try(encode(item));
        (void)rstd_co_try(co_await funcs_.sync_item.call_async_typed<luato::IgnoreReturns>(
            rstd::move(context), rstd::move(input)));
        co_return Ok(empty {});
    }
    auto sync_async(Rc<LuaCallState> call)
        -> rstd::async::coro<Result<ProviderSyncOutcome, luato::Error>> {
        auto context = rstd_co_try(co_await run_context_async(rstd::move(call)));
        auto result  = rstd_co_try(
            co_await funcs_.sync.call_async_typed<luato::MultiValue>(rstd::move(context)));
        auto values = rstd::move(result).into_values();
        if (! values.is_empty() && values[usize()].is_Integer() &&
            values[usize()].as_Integer().value == i64(2))
            co_return Ok(ProviderSyncOutcome::NotAuthenticated);
        co_return Ok(ProviderSyncOutcome::Completed);
    }
    auto favorite_async(Rc<LuaCallState> call, String native_id, i32 item_type, bool value)
        -> rstd::async::coro<Result<empty, luato::Error>> {
        auto context = rstd_co_try(co_await run_context_async(rstd::move(call)));
        (void)rstd_co_try(co_await funcs_.favorite.call_async_typed<luato::IgnoreReturns>(
            rstd::move(context), rstd::move(native_id), i64(item_type.to_primitive()), value));
        co_return Ok(empty {});
    }
    auto set_session_store(sqlito::ExecutorHandle db, i64 provider, String key) -> void {
        bindings_->session->bind(rstd::move(db), provider, rstd::move(key));
    }
    auto invalidate_session() -> void {
        bindings_->generation.invalidate();
        bindings_->session->invalidate();
        if (bindings_->http.is_some()) (*bindings_->http)->invalidate_session();
    }
    auto login_async(AuthInput input) -> rstd::async::coro<Result<luato::LuaValue, luato::Error>> {
        if (! bindings_->generation.advance())
            co_return Err(bindings::fail("provider generation exhausted"_str));
        auto info = rstd_co_try(encode(input));
        co_return co_await funcs_.login.call_async_typed<luato::LuaValue>(single_result(),
                                                                          rstd::move(info));
    }
    auto check_async() -> rstd::async::coro<Result<empty, luato::Error>> {
        (void)rstd_co_try(co_await funcs_.check.call_async_typed<luato::IgnoreReturns>());
        co_return Ok(empty {});
    }
    auto subtitle_async(String item_id)
        -> rstd::async::coro<Result<luato::LuaValue, luato::Error>> {
        return funcs_.subtitle.call_async_typed<luato::LuaValue>(
            single_result("subtitle must return one value"_str), rstd::move(item_id));
    }
    auto load_session(ref<str> data) -> Result<empty, luato::Error> {
        if (! bindings_->generation.advance())
            return Err(bindings::fail("provider generation exhausted"_str));
        bindings_->session->flights().cancel();
        if (bindings_->http.is_some()) {
            auto cookies = rstd_try((*bindings_->http)->save_cookies(), bindings::http_error);
            rstd_try((*bindings_->http)->reset_session(), bindings::http_error);
            if (! (*bindings_->http)->load_cookies(cookies.as_str()))
                return Err(bindings::fail("cannot restore provider HTTP session"_str));
        }
        saved_session_ = String::make(data);
        (void)rstd_try(funcs_.load.call<luato::IgnoreReturns>(data));
        return Ok(empty {});
    }
    auto save_session() -> Result<String, luato::Error> {
        auto result = rstd_try(
            funcs_.save.call<luato::LuaValue>(luato::ReturnOptions { Some(luato::Error::make(
                luato::ErrorKind::Module, "plugin:save"_Str, "save must return one value"_Str)) }));
        auto text = result.as_utf8();
        if (text.is_err())
            return Err(luato::Error::make(luato::ErrorKind::Module,
                                          String::make("plugin:save"_str),
                                          String::make("save must return a string"_str)));
        saved_session_ = String::make(*text);
        return Ok(saved_session_.clone());
    }
    auto take_settings_definition() -> Vec<SettingField> {
        return rstd::move(settings_definition_);
    }
    auto state() -> luato::State& { return state_; }
};
} // namespace qcm::backend

namespace qcm::backend
{
class ProviderInstance final : public Provider {
    Arc<LuaHttpBridge>  bridge_;
    Rc<LuaProviderHost> host_;
    Vec<SettingField>   settings_fields_;

    ProviderInstance(Arc<LuaHttpBridge> bridge, LuaProviderHost host)
        : bridge_(rstd::move(bridge)), host_(make_rc<LuaProviderHost>(rstd::move(host))) {}

public:
    ~ProviderInstance() override = default;
    auto into_handle() && -> ProviderHandle {
        return ProviderHandle(new ProviderInstance(rstd::move(*this)));
    }
    ProviderInstance(ProviderInstance&&)                           = default;
    auto        operator=(ProviderInstance&&) -> ProviderInstance& = default;
    static auto make(ref<Path> script, LuaProviderOptions options, RuntimeHandle runtime)
        -> ProviderResult<ProviderInstance> {
        auto initial_settings = rstd::move(options.settings);
        options.settings      = SettingsSnapshot {};
        auto bridge = rstd_try(LuaHttpBridge::make(rstd::move(runtime)), ProviderError::request);
        auto shared = Arc<LuaHttpBridge>::make(rstd::move(bridge));
        auto host   = LuaProviderHost::make(script, rstd::move(options), Some(shared.clone()));
        if (host.is_err()) return Err(rstd::into<ProviderError>(rstd::move(host).unwrap_err()));
        auto instance = ProviderInstance { rstd::move(shared), rstd::move(host).unwrap() };
        instance.settings_fields_ = instance.host_.get()->take_settings_definition();
        auto normalized           = rstd_try(instance.validate_settings(initial_settings.values));
        rstd_try(instance.set_settings(
            SettingsSnapshot { initial_settings.revision, rstd::move(normalized) }));
        return Ok(rstd::move(instance));
    }
    auto auth_async(AuthInput input) -> rstd::async::coro<ProviderResult<AuthOutcome>> override {
        auto result  = rstd_co_try(co_await host_.get()->login_async(input.clone()));
        auto outcome = rstd_co_try(host_.get()->decode_result<AuthOutcome>(rstd::move(result)));
        if (outcome.is_Ok()) rstd_co_try(set_auth_info(rstd::move(input)));
        co_return Ok(rstd::move(outcome));
    }
    auto load_session(ref<str> data) -> ProviderResult<empty> override {
        return provider_result(host_.get()->load_session(data));
    }
    auto save_session() -> ProviderResult<String> override {
        return provider_result(host_.get()->save_session());
    }
    auto save_cookie() -> ProviderResult<String> override {
        return provider_result(bridge_->save_cookies());
    }
    auto load_cookie(ref<str> data) -> ProviderResult<empty> override {
        if (! bridge_->load_cookies(data))
            return Err(ProviderError::internal("invalid provider cookies"_str));
        return Ok(empty {});
    }
    auto set_session_store(sqlito::ExecutorHandle db, i64 provider, String key) -> void override {
        host_.get()->set_session_store(rstd::move(db), provider, rstd::move(key));
    }
    auto matches_session(i64 provider, ref<str> key) const -> bool override {
        return host_.get()->matches_session(provider, key);
    }
    auto session_key() const -> Option<String> override { return host_.get()->session_key(); }
    auto resource_scope() -> ProviderResult<Option<String>> override {
        return provider_result(host_.get()->resource_scope());
    }
    auto invalidate() -> void override { host_.get()->invalidate_session(); }
    auto cancel_streams() -> void override { bridge_->cancel_streams(); }
    auto close_async() -> rstd::async::coro<empty> override {
        host_.get()->invalidate_session();
        (void)co_await bridge_->close();
        co_return empty {};
    }
    auto resource_cookie_session() const -> CookieSession override {
        return bridge_->cookie_session();
    }
    auto session_epoch() const -> Option<u64> override { return host_.get()->session_epoch(); }
    auto lifecycle_epoch() const -> u64 override { return host_.get()->lifecycle_epoch(); }
    auto generation_check() const -> ProviderGenerationCheck override {
        return host_.get()->generation_check();
    }
    auto status_snapshot() const -> ProviderCommonSnapshot override {
        return host_.get()->status_snapshot();
    }
    auto base_url() const -> String override { return host_.get()->base_url(); }
    auto auth_method() const -> Option<AuthMethod> override { return host_.get()->auth_method(); }
    template<class T, class F>
    static auto call_with_context(Rc<LuaProviderHost> host, ProviderContext context, F execute)
        -> rstd::async::coro<ProviderResult<T>> {
        if (! context.is_valid()) co_return Err(ProviderError::NotAuth());
        ProviderLibraryRun run(
            context.db.clone(),
            context.coordinator,
            context.provider_id,
            context.sync.mode == ProviderSyncMode::Explicit,
            [context = context.clone()] {
                return context.is_valid();
            },
            [context = context.clone()] {
                return context.is_writable();
            });
        rstd_co_try(run.bind_generation(host->generation_check()));
        run.set_progress([context = context.clone()](SyncProgressKind kind, i32 count) {
            return context.progress(kind, count);
        });
        auto activation = LuaCallGuard(&run);
        auto call       = activation.state();
        auto result     = co_await execute(*host.get(), rstd::move(call));
        if (! run.is_valid()) co_return Err(ProviderError::NotAuth());
        co_return provider_result(rstd::move(result));
    }
    auto sync_async(ProviderContext context, Option<Item> item)
        -> rstd::async::coro<ProviderResult<u32>> override {
        if (! context.is_valid() || session_epoch().is_none())
            co_return Err(ProviderError::NotAuth());
        bool            full            = item.is_none();
        bool            explicit_plugin = context.sync.mode == ProviderSyncMode::Explicit;
        ProviderSyncRun run(context.db.clone(),
                            context.coordinator,
                            context.provider_id,
                            explicit_plugin,
                            full && explicit_plugin,
                            context.sync.types.clone(),
                            full);
        rstd_co_try(run.bind_generation(generation_check()));
        run.set_valid([context = context.clone()] {
            return context.is_valid() && context.is_writable();
        });
        run.set_progress([context = context.clone()](SyncProgressKind kind, i32 count) {
            return context.progress(kind, count);
        });
        rstd_co_try(co_await run.start());
        auto activation = LuaCallGuard(&run);
        auto call       = activation.state();
        if (full) {
            auto outcome = rstd_co_try(co_await host_.get()->sync_async(rstd::move(call)));
            if (outcome == ProviderSyncOutcome::NotAuthenticated)
                co_return Err(ProviderError::NotAuth());
        } else {
            rstd_co_try(co_await host_.get()->sync_item_async(rstd::move(call), rstd::move(*item)));
        }
        if (! run.is_valid()) co_return Err(ProviderError::NotAuth());
        if (full) (void)rstd_co_try(co_await run.finalize());
        co_return Ok(u32(static_cast<unsigned long long>(run.songs().to_primitive())));
    }

private:
    static auto execute_resource(Rc<LuaProviderHost>                host,
                                 Arc<Option<luato::FunctionHandle>> validator,
                                 Weak<BindingsContext> owner, u64 generation,
                                 PreparedRequest request, bool streaming)
        -> rstd::async::coro<ProviderResult<ResourceResponse>> {
        Option<luato::FunctionHandle> copied;
        if (validator->is_some()) copied = Some(rstd_co_try((*validator)->clone()));
        bindings::PreparedResourceCall call { { rstd::move(request), generation },
                                              rstd::move(copied),
                                              rstd::move(owner) };
        co_return co_await host.get()->execute_prepared_async(rstd::move(call), streaming);
    }
    auto wrap_preparation(bindings::PreparedResourceCall call) -> resource_cache::PreparedResource {
        auto generation = call.resource.generation;
        auto validator  = Arc<Option<luato::FunctionHandle>>::make(rstd::move(call.validator));
        auto fetch      = resource_cache::PreparedResource::Fetch::make(
            [host      = host_.clone(),
             validator = rstd::move(validator),
             owner     = rstd::move(call.owner),
             generation](PreparedRequest request, bool streaming) {
                return execute_resource(host.clone(),
                                        validator.clone(),
                                        owner.clone(),
                                        generation,
                                        rstd::move(request),
                                        streaming);
            });
        auto current = resource_cache::PreparedResource::Check::make(
            [owner = host_.clone(), generation = host_->generation_check()] {
                return owner->session_epoch().is_some() && generation.is_current();
            });
        return resource_cache::PreparedResource(rstd::move(call.resource.request),
                                                rstd::move(fetch),
                                                bridge_->cookie_session(),
                                                None(),
                                                rstd::move(current));
    }

public:
    auto prepare_playback_resource_async(ref<str> state, ref<str> uri,
                                         const lihttpto::Headers& headers)
        -> rstd::async::coro<ProviderResult<resource_cache::PreparedResource>> override {
        auto plan =
            rstd_co_try(co_await host_.get()->prepare_playback_call_async(state, uri, headers));
        co_return Ok(wrap_preparation(rstd::move(plan)));
    }
    auto prepare_resource_async(Resource resource, const lihttpto::Headers& headers)
        -> rstd::async::coro<ProviderResult<resource_cache::PreparedResource>> override {
        auto plan = rstd_co_try(
            co_await host_.get()->prepare_resource_async(rstd::move(resource), headers));
        co_return Ok(wrap_preparation(rstd::move(plan)));
    }
    auto for_you_items_async(ProviderContext context)
        -> rstd::async::coro<ProviderResult<Vec<HomeEntry>>> override {
        return call_with_context<Vec<HomeEntry>>(
            host_.clone(), rstd::move(context), [](LuaProviderHost& host, Rc<LuaCallState> call) {
                return host.for_you_items_async(rstd::move(call));
            });
    }
    auto open_playback_async(PlaybackRequest request)
        -> rstd::async::coro<ProviderResult<PreparedPlayback>> override {
        co_return co_await provider_result(host_.get()->open_playback_async(rstd::move(request)));
    }
    auto keep_playback_async(String state) -> rstd::async::coro<ProviderResult<empty>> override {
        return provider_result(host_.get()->keep_playback_async(rstd::move(state)));
    }
    auto close_playback_async(String state) -> rstd::async::coro<ProviderResult<empty>> override {
        return provider_result(host_.get()->close_playback_async(rstd::move(state)));
    }

private:
    auto comment_page(ProviderContext context, provider_comment::CommentTarget target,
                      Option<String> parent, provider_comment::CommentPageRequest request)
        -> rstd::async::coro<ProviderResult<Option<provider_comment::CommentPage>>> {
        auto epoch = lifecycle_epoch();
        auto page  = rstd_co_try(co_await call_with_context<Option<provider_comment::CommentPage>>(
            host_.clone(),
            rstd::move(context),
            [target = rstd::move(target), parent = rstd::move(parent), request = request.clone()](
                LuaProviderHost& host, Rc<LuaCallState> call) mutable {
                return host.comments_async(
                    rstd::move(call), rstd::move(target), rstd::move(parent), rstd::move(request));
            }));
        if (lifecycle_epoch() != epoch) co_return Err(ProviderError::NotAuth());
        if (page.is_some()) rstd_co_try(provider_comment::validate_page(*page, request));
        co_return Ok(rstd::move(page));
    }

public:
    auto comments_async(ProviderContext context, provider_comment::CommentTarget target,
                        provider_comment::CommentPageRequest request)
        -> rstd::async::coro<ProviderResult<Option<provider_comment::CommentPage>>> override {
        return comment_page(rstd::move(context), rstd::move(target), None(), rstd::move(request));
    }
    auto comment_replies_async(ProviderContext context, provider_comment::CommentTarget target,
                               String parent, provider_comment::CommentPageRequest request)
        -> rstd::async::coro<ProviderResult<Option<provider_comment::CommentPage>>> override {
        return comment_page(
            rstd::move(context), rstd::move(target), Some(rstd::move(parent)), rstd::move(request));
    }
    auto set_comment_like_async(ProviderContext context, provider_comment::CommentTarget target,
                                String comment_id, bool value)
        -> rstd::async::coro<ProviderResult<provider_comment::CommentLikeResult>> override {
        auto epoch  = lifecycle_epoch();
        auto result = rstd_co_try(co_await call_with_context<provider_comment::CommentLikeResult>(
            host_.clone(),
            rstd::move(context),
            [target = rstd::move(target), comment_id = rstd::move(comment_id), value](
                LuaProviderHost& host, Rc<LuaCallState> call) mutable {
                return host.set_comment_like_async(
                    rstd::move(call), rstd::move(target), rstd::move(comment_id), value);
            }));
        if (lifecycle_epoch() != epoch) co_return Err(ProviderError::NotAuth());
        if (result.liked != value)
            co_return Err(ProviderError::internal("unexpected comment reaction"_str));
        co_return Ok(rstd::move(result));
    }
    auto home_blocks_async(ProviderContext context)
        -> rstd::async::coro<ProviderResult<Vec<HomeBlock>>> override {
        return call_with_context<Vec<HomeBlock>>(
            host_.clone(), rstd::move(context), [](LuaProviderHost& host, Rc<LuaCallState> call) {
                return host.home_blocks_async(rstd::move(call));
            });
    }
    auto home_block_items_async(ProviderContext context, String block_id, i32 page, i32 page_size)
        -> rstd::async::coro<ProviderResult<HomeBlockItems>> override {
        return call_with_context<HomeBlockItems>(
            host_.clone(),
            rstd::move(context),
            [block_id = rstd::move(block_id), page, page_size](
                LuaProviderHost& host, Rc<LuaCallState> call) mutable {
                return host.home_block_items_async(
                    rstd::move(call), rstd::move(block_id), page, page_size);
            });
    }
    auto qr_async() -> rstd::async::coro<ProviderResult<QrInfo>> override {
        return provider_result(host_.get()->qr_async());
    }
    auto share_url(ref<str> native_id, ItemType item_type)
        -> ProviderResult<Option<lihttpto::Url>> override {
        auto value = rstd_try(host_.get()->share_url(native_id, i64(static_cast<int>(item_type))));
        if (value.is_none()) return Ok(None());
        auto url = lihttpto::Url::parse_http(value->as_str());
        if (url.is_err()) return Err(ProviderError::internal("invalid share URL"_str));
        return Ok(Some(rstd::move(url).unwrap()));
    }
    auto queue_definition_async(ProviderContext context, String native_id)
        -> rstd::async::coro<ProviderResult<RadioDefinition>> override {
        return call_with_context<RadioDefinition>(
            host_.clone(),
            rstd::move(context),
            [native_id =
                 rstd::move(native_id)](LuaProviderHost& host, Rc<LuaCallState> call) mutable {
                return host.queue_definition_async(rstd::move(call), rstd::move(native_id));
            });
    }
    auto open_queue_async(ProviderContext context, String native_id, QueueOpenRequest request)
        -> rstd::async::coro<ProviderResult<RadioBatch>> override {
        return call_with_context<RadioBatch>(
            host_.clone(),
            rstd::move(context),
            [native_id = rstd::move(native_id),
             request = rstd::move(request)](LuaProviderHost& host, Rc<LuaCallState> call) mutable {
                return host.open_queue_async(
                    rstd::move(call), rstd::move(native_id), rstd::move(request));
            });
    }
    auto next_queue_async(ProviderContext context, String native_id, QueueNextRequest request)
        -> rstd::async::coro<ProviderResult<RadioBatch>> override {
        return call_with_context<RadioBatch>(
            host_.clone(),
            rstd::move(context),
            [native_id = rstd::move(native_id),
             request = rstd::move(request)](LuaProviderHost& host, Rc<LuaCallState> call) mutable {
                return host.next_queue_async(
                    rstd::move(call), rstd::move(native_id), rstd::move(request));
            });
    }
    auto legacy_queue_async(ProviderContext context, String native_id, Vec<String> ids,
                            QueueConfiguration configuration)
        -> rstd::async::coro<ProviderResult<RadioBatch>> override {
        return call_with_context<RadioBatch>(
            host_.clone(),
            rstd::move(context),
            [native_id = rstd::move(native_id),
             ids       = rstd::move(ids),
             configuration =
                 rstd::move(configuration)](LuaProviderHost& host, Rc<LuaCallState> call) mutable {
                return host.legacy_queue_async(rstd::move(call),
                                               rstd::move(native_id),
                                               rstd::move(ids),
                                               rstd::move(configuration),
                                               None());
            });
    }
    auto close_queue_async(ProviderContext context, String native_id, String state)
        -> rstd::async::coro<ProviderResult<empty>> override {
        return call_with_context<empty>(
            host_.clone(),
            rstd::move(context),
            [native_id = rstd::move(native_id),
             state     = rstd::move(state)](LuaProviderHost& host, Rc<LuaCallState> call) mutable {
                return host.close_queue_async(
                    rstd::move(call), rstd::move(native_id), rstd::move(state));
            });
    }
    auto decode_subtitle(luato::LuaValue entry) -> ProviderResult<SubtitleData> {
        if (entry.is_Bytes()) {
            auto text = entry.as_utf8();
            if (text.is_err())
                return Err(
                    ProviderError::ParseSubtitle(String::make("subtitle must be valid text"_str)));
            auto parsed = parse_lrc(*text);
            if (parsed.is_err())
                return Err(ProviderError::ParseSubtitle(
                    rstd::format("{:?}", parsed.as_ref().unwrap_err())));
            return Ok(rstd::move(*parsed));
        }
        if (! entry.is_Table()) return Err(ProviderError::NotFound());
        auto data =
            rstd_try(provider_result(host_->decode_result<SubtitleData>(rstd::move(entry))));
        if (data.items.len() > usize(100000))
            return Err(ProviderError::ParseSubtitle("invalid structured subtitle"_Str));
        for (const auto& row : data.items)
            if ((row.start.is_some() && *row.start < i64()) ||
                (row.end.is_some() && *row.end < (row.start.is_some() ? *row.start : i64())) ||
                (row.text.is_some() && row.text->len() > usize(1024 * 1024)))
                return Err(ProviderError::ParseSubtitle("invalid structured subtitle"_Str));
        return Ok(rstd::move(data));
    }
    auto subtitle_async(String native_id)
        -> rstd::async::coro<ProviderResult<SubtitleData>> override {
        auto result = rstd_co_try(co_await host_.get()->subtitle_async(rstd::move(native_id)));
        co_return decode_subtitle(rstd::move(result));
    }
    auto settings_definition() const -> slice<SettingField> override {
        return settings_fields_.as_slice();
    }
    auto set_settings(SettingsSnapshot values) -> ProviderResult<empty> override {
        return provider_result(host_.get()->set_settings(rstd::move(values)));
    }
    auto settings() const -> SettingsSnapshot override { return host_.get()->settings(); }
    auto set_name(ref<str> name) -> ProviderResult<empty> override {
        return provider_result(host_.get()->set_name(name));
    }
    auto set_identity(i64 id, ref<str> name) -> ProviderResult<empty> override {
        return provider_result(host_.get()->set_identity(id, name));
    }
    auto set_auth_info(AuthInput input) -> ProviderResult<empty> override {
        return provider_result(host_.get()->set_auth_info(rstd::move(input)));
    }
    auto validate_settings(const SettingsValues& values)
        -> Result<SettingsValues, SettingsError> override {
        SettingsError error;
        auto          normalized =
            rstd_try(qcm::backend::validate_settings(settings_fields_.as_slice(), values));
        auto custom = host_.get()->validate_settings_value(normalized);
        if (custom.is_err()) {
            error.message = rstd::format("{}", custom.as_ref().unwrap_err());
            error.cause = Some(Arc<ErrorCause>::make(error_cause(rstd::move(custom).unwrap_err())));
            return Err(rstd::move(error));
        }
        if (custom->is_some()) {
            normalized =
                rstd_try(qcm::backend::validate_settings(settings_fields_.as_slice(), **custom));
        }
        return Ok(rstd::move(normalized));
    }
    auto favorite_async(ProviderContext context, String native_id, ItemType item_type, bool value)
        -> rstd::async::coro<ProviderResult<empty>> override {
        return call_with_context<empty>(host_.clone(),
                                        rstd::move(context),
                                        [native_id = rstd::move(native_id), item_type, value](
                                            LuaProviderHost& host, Rc<LuaCallState> call) mutable {
                                            return host.favorite_async(
                                                rstd::move(call),
                                                rstd::move(native_id),
                                                i32(static_cast<int>(item_type)),
                                                value);
                                        });
    }
};
static_assert(! __is_abstract(ProviderInstance));
} // namespace qcm::backend

export namespace qcm::backend
{
auto make_lua_provider(ref<Path> script, ProviderOptions options)
    -> ProviderResult<ProviderHandle> {
    if (options.runtime.is_none())
        return Err(ProviderError::internal("provider runtime is unavailable"_str));
    LuaProviderOptions lua;
    lua.type_name    = rstd::move(options.type_name);
    lua.name         = rstd::move(options.name);
    lua.base_url     = rstd::move(options.base_url);
    lua.settings     = rstd::move(options.settings);
    lua.auth_method  = rstd::move(options.auth_method);
    lua.device_id    = rstd::move(options.device_id);
    lua.config       = rstd::move(options.config);
    lua.installation = rstd::move(options.installation);
    lua.id           = options.id;
    lua.playing      = rstd::move(options.playing);
    auto instance =
        rstd_try(ProviderInstance::make(script, rstd::move(lua), rstd::move(*options.runtime)));
    return Ok(rstd::move(instance).into_handle());
}
} // namespace qcm::backend
