module;
#include <rstd/macro.hpp>
export module qcm.backend:api.resource_http;
export import :api.http;
import :model.item;
import rstd.log;
using rstd::async::coro;
using rstd::async::CoroFuture;
using rstd::bytes::Bytes;
using rstd::sync::Arc;
using rstd::task::Poll;
using rstd::time::Duration;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
auto resource_request_headers(const liserveto::Request& request) -> lihttpto::Headers {
    lihttpto::Headers headers;
    for (const auto& field : request.head().headers.as_slice()) {
        auto file_header = field.name.matches("qextra-image-transport"_str) ||
                           field.name.matches("qcm-file-client"_str);
        if (resource_header_allowed(field.name) || file_header) headers.push(field.clone());
    }
    return headers;
}

struct ResourceFrames {
    using Item = liserveto::BodyItem;
    resource_cache::ResourceBody                                     source;
    Option<CoroFuture<Result<Option<Bytes>, resource_cache::Error>>> reading;
    Option<CoroFuture<empty>>                                        closing;
    bool                                                             done {};

    auto poll_next(mut_ref<ResourceFrames> self, rstd::task::Context& cx) -> Poll<Option<Item>> {
        using Output = Poll<Option<Item>>;
        if (self->done) return Output::Ready(None());
        if (self->reading.is_none())
            self->reading = Some(rstd::async::into_future(self->source.next()));
        auto result = rstd::future::poll(*self->reading, cx);
        if (result.is_pending()) return Output::Pending();
        auto chunk    = rstd::move(result).take();
        self->reading = None();
        if (chunk.is_err()) {
            rstd::log::warn("resource body failed: {}", chunk.as_ref().unwrap_err());
            self->done = true;
            self->source.cancel();
            return Output::Ready(Some(Item(Err(liserveto::BodyError::Source))));
        }
        if (chunk->is_none()) {
            self->done = true;
            return Output::Ready(None());
        }
        return Output::Ready(Some(Item(Ok(lihttpto::BodyFrame::Data(rstd::move(**chunk))))));
    }

    auto poll_close(mut_ref<ResourceFrames> self, rstd::task::Context& cx)
        -> Poll<Result<empty, liserveto::BodyError>> {
        using Output = Poll<Result<empty, liserveto::BodyError>>;
        if (self->closing.is_none()) {
            self->done = true;
            self->source.cancel();
            self->reading = None();
            self->closing = Some(rstd::async::into_future(self->source.close()));
        }
        auto result = rstd::future::poll(*self->closing, cx);
        if (result.is_pending()) return Output::Pending();
        self->closing = None();
        return Output::Ready(Ok(empty {}));
    }
};

auto resource_response(resource_cache::Response source) -> liserveto::Response {
    auto length = source.content_length();
    auto body =
        liserveto::Body::from_stream(ResourceFrames { rstd::move(source.body), {}, {}, false });
    auto response        = liserveto::Response::stream(rstd::move(body), length);
    auto status          = lihttpto::StatusCode::make(u16(source.status.to_primitive()));
    response.head.status = status.is_ok() ? *status : lihttpto::StatusCode::make(u16(502)).unwrap();
    response.head.headers = resource_cache::forwarded(source.headers);
    response.head.headers.remove("content-length"_str);
    response.write_timeout = Some(Duration::from_secs(u64(30)));
    response.idle_timeout  = Some(Duration::from_secs(u64(30)));
    return response;
}

auto resource_response(resource_cache::ResourceDelivery delivery) -> HttpResponseResult {
    auto response = rstd_try(rstd::move(delivery).into_response(), [](auto error) {
        return ProcessError::from_resource(rstd::move(error));
    });
    return Ok(resource_response(rstd::move(response)));
}

auto resource_file_lease_http(Arc<QcmService> service, liserveto::Request request)
    -> coro<HttpResponseResult> {
    auto response =
        co_await service->file_lease_response(request.head().line.method.as_str(),
                                              request.head().line.resource.escaped_path.as_str(),
                                              request.head().headers);
    if (response.is_none()) co_return Err(ProcessError::NotFound());
    co_return Ok(resource_response(rstd::move(*response)));
}

auto image_http(Arc<QcmService> service, liserveto::Request request) -> coro<HttpResponseResult> {
    auto item_type  = parameter_text(request, "item_type"_str);
    auto image_type = parameter_text(request, "image_type"_str);
    auto id_text    = parameter_text(request, "id"_str);
    auto id         = parse_i64(id_text.as_str());
    if (id.is_none()) co_return Err(ProcessError::NotFound());
    auto headers = resource_request_headers(request);
    auto plan    = rstd_co_try(co_await service->image_resource_plan(
        *id, item_type.as_str(), image_type.as_str(), headers));
    if (plan.is_none()) co_return Err(ProcessError::NotFound());
    auto delivery = rstd_co_try(
        co_await service->object_resource(rstd::move(*plan), rstd::move(headers)), [](auto error) {
            return ProcessError::from_resource(rstd::move(error));
        });
    co_return resource_response(rstd::move(delivery));
}

auto audio_http(Arc<QcmService> service, liserveto::Request request) -> coro<HttpResponseResult> {
    auto item_type = parameter_text(request, "item_type"_str);
    auto type      = rstd::from_str<ItemType>(item_type.as_str()).ok();
    if (type.is_none()) co_return Err(ProcessError::NoSuchItemType(rstd::move(item_type)));
    if (*type != ItemType::Song)
        co_return Err(ProcessError::UnsupportedItemType(rstd::move(item_type)));
    auto id_text = parameter_text(request, "id"_str);
    auto id      = parse_i64(id_text.as_str());
    if (id.is_none()) co_return Err(ProcessError::NotFound());
    auto headers  = resource_request_headers(request);
    auto prepared = rstd_co_try(co_await service->audio_resource_plan(*id, headers));
    if (prepared.is_none()) co_return Err(ProcessError::NotFound());
    auto delivery =
        rstd_co_try(co_await service->object_resource(rstd::move(*prepared), rstd::move(headers)),
                    [](auto error) {
                        return ProcessError::from_resource(rstd::move(error));
                    });
    co_return resource_response(rstd::move(delivery));
}

auto install_resource_http_routes(liserveto::Router& router, Arc<QcmService> service) -> void {
    router
        .mount("/resource-file-leases"_str,
               qcm_http_endpoint([service = service.clone()](liserveto::Request request) {
                   return resource_file_lease_http(service.clone(), rstd::move(request));
               }))
        .unwrap();
    router
        .get("/image/{item_type}/{id}/{image_type}"_str,
             qcm_http_endpoint([service = service.clone()](liserveto::Request request) {
                 return image_http(service.clone(), rstd::move(request));
             }))
        .unwrap();
    router
        .get("/audio/{item_type}/{id}"_str,
             qcm_http_endpoint([service = service.clone()](liserveto::Request request) {
                 return audio_http(service.clone(), rstd::move(request));
             }))
        .unwrap();
}

auto dispatch_resource_http(Arc<QcmService> service, liserveto::Endpoint next,
                            liserveto::Request request) -> coro<liserveto::ResponseResult> {
    if (request.head().line.method.as_str() == "POST"_str) {
        auto response = co_await service->file_lease_response(
            request.head().line.method.as_str(),
            request.head().line.resource.escaped_path.as_str(),
            request.head().headers);
        if (response.is_some()) co_return Ok(resource_response(rstd::move(*response)));
    }
    co_return co_await next.call(rstd::move(request));
}

auto resource_http_dispatch(Arc<QcmService> service, liserveto::Endpoint next)
    -> liserveto::Endpoint {
    return liserveto::Endpoint::make(
        [service = rstd::move(service), next = rstd::move(next)](liserveto::Request request) {
            return dispatch_resource_http(service.clone(), next.clone(), rstd::move(request));
        });
}
} // namespace qcm::backend
