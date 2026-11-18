module;
#include <rstd/macro.hpp>
module qcm.backend;
import :api.playback_http;
import :api.http;
import :service;
import :playback;
import liserveto;
import :api.resource_http;
import rstd.json;
using rstd::async::coro;
using rstd::bytes::Bytes;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
static auto json_bytes_response(String body) -> liserveto::Response {
    auto response = liserveto::Response::bytes(Bytes::copy_from_slice(body.as_str().as_bytes()));
    response.head.status = lihttpto::StatusCode::make(u16(200)).unwrap();
    (void)response.head.headers.add("content-type"_str, "application/json"_str);
    (void)response.head.headers.add("cache-control"_str, "no-store"_str);
    return response;
}

static auto playback_source_json(PlaybackSource source) -> String {
    using Value = rstd::json::Value;
    rstd::json::Map fields;
    fields.insert(String::make("url"_str), Value::String(rstd::move(source.url)));
    fields.insert(String::make("session"_str),
                  source.session.is_some() ? Value::String(rstd::move(*source.session))
                                           : Value::Null());
    fields.insert(String::make("time_seekable"_str), Value::Bool(source.time_seekable));
    if (source.time_seekable) {
        auto duration = source.duration_seconds.is_some()
                            ? rstd::json::Number::from_f64(*source.duration_seconds)
                            : None<rstd::json::Number>();
        fields.insert(String::make("duration_seconds"_str),
                      duration.is_some() ? Value::Number(rstd::move(*duration)) : Value::Null());
    }
    return rstd::json::to_string(Value::Object(rstd::move(fields)));
}

auto playback_body_json(liserveto::Request& request, bool allow_empty)
    -> coro<ProcessResult<rstd::json::Value>> {
    auto collected = rstd_co_try(co_await request.collect(u64(65536)), [](auto error) {
        return ProcessError::http_body(rstd::move(error));
    });
    if (allow_empty && collected.data.is_empty())
        co_return Ok(rstd::json::Value::Object(rstd::json::Map {}));
    auto text   = rstd_co_try(rstd::str_::from_utf8(collected.data.as_slice()), [](auto error) {
        return ProcessError::internal(rstd::move(error));
    });
    auto parsed = rstd_co_try(rstd::json::from_str(text), [](auto error) {
        return ProcessError::internal(rstd::move(error));
    });
    if (! parsed.is_Object())
        co_return Err(ProcessError::WrongId(String::make("request body"_str)));
    co_return Ok(rstd::move(parsed));
}

auto playback_source_http_request(liserveto::Request& request)
    -> coro<ProcessResult<PlaybackSourceHttpRequest>> {
    auto id_text = parameter_text(request, "id"_str);
    auto id      = parse_i64(id_text.as_str());
    if (id.is_none()) co_return Err(ProcessError::NotFound());
    auto        parsed = rstd_co_try(co_await playback_body_json(request, true));
    Option<i32> max_bitrate;
    auto        value = parsed.get("max_bitrate"_str);
    if (value.is_some() && (*value)->as_i64().is_some()) {
        auto raw = i64((*(*value)->as_i64()).to_primitive());
        if (raw < i64() || raw > i64(2147483647LL)) co_return Err(ProcessError::NotFound());
        max_bitrate = Some(i32(static_cast<int>(raw.to_primitive())));
    }
    co_return Ok(PlaybackSourceHttpRequest { *id, max_bitrate });
}

auto playback_resolve_http_request(liserveto::Request& request)
    -> coro<ProcessResult<PlaybackResolveHttpRequest>> {
    auto token  = parameter_text(request, "token"_str);
    auto parsed = rstd_co_try(co_await playback_body_json(request, false));
    auto uri    = parsed.get("uri"_str);
    if (uri.is_none() || (*uri)->as_str().is_none()) co_return Err(ProcessError::NotFound());
    co_return Ok(PlaybackResolveHttpRequest { rstd::move(token), String::make(*(*uri)->as_str()) });
}

static auto playback_source_http(Arc<QcmService> service, liserveto::Request request)
    -> coro<HttpResponseResult> {
    auto input = rstd_co_try(co_await playback_source_http_request(request));
    auto source =
        rstd_co_try(co_await service->playback().source(input.song_id, input.max_bitrate));
    if (source.is_none()) co_return Err(ProcessError::NotFound());
    co_return Ok(json_bytes_response(playback_source_json(rstd::move(*source))));
}

static auto playback_close_http(Arc<QcmService> service, liserveto::Request request)
    -> coro<HttpResponseResult> {
    auto token = parameter_text(request, "token"_str);
    service->playback().close(token.as_str());
    co_return Ok(json_bytes_response(String::make("{}"_str)));
}

static auto playback_touch_http(Arc<QcmService> service, liserveto::Request request)
    -> coro<HttpResponseResult> {
    auto token = parameter_text(request, "token"_str);
    if (! service->playback().touch(token.as_str())) co_return Err(ProcessError::NotFound());
    co_return Ok(json_bytes_response(String::make("{}"_str)));
}

static auto playback_resolve_http(Arc<QcmService> service, liserveto::Request request)
    -> coro<HttpResponseResult> {
    auto input = rstd_co_try(co_await playback_resolve_http_request(request));
    auto url   = rstd_co_try(
        co_await service->playback().resolve(rstd::move(input.token), rstd::move(input.uri)));
    if (url.is_none()) co_return Err(ProcessError::NotFound());
    auto encoded = rstd::json::to_string(rstd::json::Value::String(rstd::move(*url)));
    co_return Ok(json_bytes_response(rstd::format("{{\"url\":{}}}", encoded.as_str())));
}

auto playback_resource_http_request(const liserveto::Request& request)
    -> ProcessResult<PlaybackResourceHttpRequest> {
    auto method = request.head().line.method.as_str();
    if (method != "GET"_str && method != "HEAD"_str) return Err(ProcessError::NotFound());
    auto target = request.head().line.target.as_str();
    auto prefix = "/playback/"_str;
    if (target.as_bytes().len().to_primitive() <= prefix.as_bytes().len().to_primitive() ||
        ! target.starts_with(prefix))
        return Err(ProcessError::NotFound());
    auto token_end = text_byte_index(target, '/', prefix.as_bytes().len().to_primitive());
    if (token_end.is_none()) return Err(ProcessError::NotFound());
    auto token = text_byte_slice(target, prefix.as_bytes().len().to_primitive(), *token_end);
    auto rest  = text_byte_slice(target, *token_end, target.as_bytes().len().to_primitive());
    if (! rest.as_str().starts_with("/r/"_str)) return Err(ProcessError::NotFound());
    auto index_end = text_byte_index(rest.as_str(), '/', 3);
    if (index_end.is_none()) return Err(ProcessError::NotFound());
    auto index_text = text_byte_slice(rest.as_str(), 3, *index_end);
    auto index      = parse_i64(index_text.as_str());
    if (index.is_none() || *index < i64()) return Err(ProcessError::NotFound());
    auto path_and_query =
        text_byte_slice(rest.as_str(), *index_end, rest.as_str().as_bytes().len().to_primitive());
    auto   query_mark = text_byte_index(path_and_query.as_str(), '?', 0);
    String path, query;
    if (query_mark.is_some()) {
        path  = text_byte_slice(path_and_query.as_str(), 0, *query_mark);
        query = text_byte_slice(path_and_query.as_str(),
                                *query_mark + 1,
                                path_and_query.as_str().as_bytes().len().to_primitive());
    } else {
        path = path_and_query.clone();
    }
    return Ok(PlaybackResourceHttpRequest { rstd::move(token),
                                            usize(static_cast<rstd::size_t>(index->to_primitive())),
                                            rstd::move(path),
                                            rstd::move(query),
                                            method == "HEAD"_str });
}

static auto playback_resource_http(Arc<QcmService> service, liserveto::Request request)
    -> coro<HttpResponseResult> {
    auto input    = rstd_co_try(playback_resource_http_request(request));
    auto headers  = resource_request_headers(request);
    auto delivery = rstd_co_try(co_await service->resolve_playback_async(rstd::move(input.token),
                                                                         input.index,
                                                                         rstd::move(input.path),
                                                                         rstd::move(input.query),
                                                                         rstd::move(headers)));
    if (delivery.is_none()) co_return Err(ProcessError::NotFound());
    auto response = rstd_co_try(resource_response(rstd::move(*delivery)));
    if (input.head) response.body = liserveto::Body {};
    co_return Ok(rstd::move(response));
}

auto install_playback_http_routes(liserveto::Router& router, Arc<QcmService> service) -> void {
    router
        .post("/playback/source/Song/{id}"_str,
              qcm_http_endpoint([service = service.clone()](liserveto::Request request) {
                  return playback_source_http(service.clone(), rstd::move(request));
              }))
        .unwrap();
    router
        .post("/playback/{token}/close"_str,
              qcm_http_endpoint([service = service.clone()](liserveto::Request request) {
                  return playback_close_http(service.clone(), rstd::move(request));
              }))
        .unwrap();
    router
        .post("/playback/{token}/touch"_str,
              qcm_http_endpoint([service = service.clone()](liserveto::Request request) {
                  return playback_touch_http(service.clone(), rstd::move(request));
              }))
        .unwrap();
    router
        .post("/playback/{token}/resolve"_str,
              qcm_http_endpoint([service = service.clone()](liserveto::Request request) {
                  return playback_resolve_http(service.clone(), rstd::move(request));
              }))
        .unwrap();
    router
        .mount("/playback"_str,
               qcm_http_endpoint([service = service.clone()](liserveto::Request request) {
                   return playback_resource_http(service.clone(), rstd::move(request));
               }))
        .unwrap();
}
} // namespace qcm::backend
