module;
#include <rstd/macro.hpp>
module qcm.backend;
import :resource.service;
import :resource.store;
import licrypto;
import rstd.log;
using namespace rstd::prelude;
using namespace rstd::literals;
using lihttpto::Headers;
using rstd::tuple;
using rstd::async::coro;
using rstd::async::OwnedSemaphorePermit;
using rstd::bytes::Bytes;
using rstd::bytes::BytesMut;
using rstd::sync::Arc;

namespace qcm::backend::resource_cache
{
struct ResourceCache::ObjectBody {
    Bytes                        bytes;
    Arc<PreparedResource>        prepared;
    Option<OwnedSemaphorePermit> reader;
    Option<OwnedSemaphorePermit> buffer;
    ObjectBody(Bytes bytes, Arc<PreparedResource> prepared, OwnedSemaphorePermit reader,
               OwnedSemaphorePermit buffer)
        : bytes(rstd::move(bytes)),
          prepared(rstd::move(prepared)),
          reader(Some(rstd::move(reader))),
          buffer(Some(rstd::move(buffer))) {}
    void cancel() {
        bytes  = {};
        reader = None();
        buffer = None();
    }
    auto close() -> coro<empty> {
        cancel();
        co_return empty {};
    }
    using State = tuple<Arc<ObjectBody>, Range>;
    static auto advance(Option<State> state)
        -> coro<Option<tuple<ServiceResult<Bytes>, Option<State>>>> {
        if (state.is_none()) co_return None();
        auto& [owner, range] = *state;
        if (range.start == range.end || owner->reader.is_none()) {
            owner->cancel();
            co_return None();
        }
        if (! owner->prepared->is_valid()) {
            owner->cancel();
            co_return Some(
                tuple<ServiceResult<Bytes>, Option<State>> { Err(Error::invalidated()), None() });
        }
        auto end =
            rstd::cmp::min(range.end, range.start.saturating_add(u64(OUTPUT_BYTES.to_primitive())));
        auto chunk =
            owner->bytes.slice(usize(range.start.to_primitive()), usize(end.to_primitive()));
        range.start = end;
        co_return Some(tuple<ServiceResult<Bytes>, Option<State>> { Ok(rstd::move(chunk)),
                                                                    rstd::move(state) });
    }
    static auto stream(Arc<ObjectBody> owner, Range range) {
        return rstd::stream::unfold(Option<State>(Some(State { rstd::move(owner), range })),
                                    [](Option<State> state) {
                                        return advance(rstd::move(state)).into_future();
                                    });
    }
};

auto ResourceCache::respond_object(ResourceRequest request)
    -> coro<ServiceResult<ResourceDelivery>> {
    if (*stop.borrow() || ! request.prepared->is_valid()) co_return Err(Error::invalidated());
    auto           permit   = rstd_co_try(co_await readers.clone().acquire_owned(), acquire_error);
    auto           buffer   = rstd_co_try(co_await buffers.clone().acquire_owned(), acquire_error);
    auto           prepared = request.prepared.clone();
    auto           base     = request_headers(prepared->headers(), request.headers);
    auto           key      = rstd_co_try(request_identity(request, base));
    auto           opening  = co_await open_lock(key.as_str());
    auto           opened   = co_await opening->lock();
    bool           bypass   = request_no_store(request.headers);
    Option<Object> cached;
    if (! bypass && store.is_some()) {
        auto found = co_await storage().object(key.clone());
        if (found.is_ok())
            cached = rstd::move(found).unwrap();
        else
            rstd::log::warn("object cache lookup failed: {}", found.as_ref().unwrap_err());
    }
    bool fresh = cached.is_some() && cached->meta.policy.fresh(request.headers);
    if (only_cached(request.headers) && ! fresh) {
        auto response          = empty_response(i32(504));
        response.effective_url = Some(String::make(prepared->url()));
        co_return Ok(ResourceDelivery::Body(rstd::move(response)));
    }
    Option<Object> object;
    if (fresh)
        object = cached.take();
    else {
        auto headers = base.clone();
        if (cached.is_some()) {
            auto conditions = cached->meta.policy.conditional();
            for (const auto& field : conditions.as_slice())
                (void)headers.set(field.name.as_str(), field.value.clone());
        }
        auto sent      = now();
        auto transfer  = rstd_co_try(co_await requests.clone().acquire_owned(), acquire_error);
        auto response  = rstd_co_try(co_await execute(prepared.clone(), rstd::move(headers)));
        auto effective = response.effective_url.is_some() ? response.effective_url->clone()
                                                          : String::make(prepared->url());
        if (response.status == i32(304)) {
            if (cached.is_none()) co_return Err(Error::state("unexpected object 304"_str));
            auto old    = rstd::move(cached).unwrap();
            auto target = rstd_co_try(target_identity(*prepared, response));
            if (target != old.meta.target)
                co_return Err(Error::state("object validation target changed"_str));
            static constexpr ref<str> names[] = { "etag"_str, "last-modified"_str };
            for (auto name : names) {
                auto        actual = value(response.headers, name);
                const auto& expected =
                    name == "etag"_str ? old.meta.policy.etag : old.meta.policy.modified;
                if (actual.is_some() && (expected.is_none() || expected->as_str() != *actual))
                    co_return Err(Error::state("object validator changed"_str));
            }
            auto length = value(response.headers, "content-length"_str);
            if (length.is_some()) {
                auto parsed = rstd_co_try(rstd::from_str<u64>(*length), [](auto error) {
                    return Error::state("invalid object validation length"_str, rstd::move(error));
                });
                if (parsed != old.meta.length)
                    co_return Err(Error::state("object validation length changed"_str));
            }
            auto merged      = merge_validation(old.meta.headers, response.headers);
            old.meta.policy  = Policy::from_response(merged, sent, now());
            old.meta.headers = saved(merged);
            if (store.is_some()) {
                if (old.meta.policy.store)
                    (void)co_await storage().update_object(old.meta.clone());
                else
                    (void)co_await storage().remove_object(key.clone());
            }
            object = Some(rstd::move(old));
            (void)co_await response.close();
        } else {
            cached        = None();
            auto expected = response.content_length();
            if (response.status != i32(200) || (expected.is_some() && *expected > MAX_OBJECT)) {
                if (response.status == i32(200) && request.headers.contains("range"_str)) {
                    (void)co_await response.close();
                    response = rstd_co_try(co_await execute(
                        prepared.clone(), downstream_request(base, request.headers)));
                }
                co_return Ok(passthrough(rstd::move(response),
                                         rstd::move(permit),
                                         prepared.clone(),
                                         Some(rstd::move(buffer)),
                                         Some(rstd::move(transfer))));
            }
            auto     policy        = Policy::from_response(response.headers, sent, now());
            auto     saved_headers = saved(response.headers);
            BytesMut data;
            for (;;) {
                auto selected = co_await rstd::async::select(
                    rstd::async::timeout(response.next().into_future(),
                                         rstd::time::Duration::from_secs(u64(30))),
                    rstd::async::select(prepared->cancelled().into_future(),
                                        wait_stopped().into_future()));
                if (selected.is_right()) co_return Err(Error::invalidated());
                auto timed = rstd_co_try(rstd::move(selected).unwrap_left(),
                                         Error::request(lifetchto::Error::Timeout()));
                auto chunk = rstd_co_try(timed);
                if (chunk.is_none()) break;
                if (! prepared->is_valid() || *stop.borrow()) co_return Err(Error::invalidated());
                if (u64(data.len().to_primitive()) + u64(chunk->len().to_primitive()) >
                    MAX_OBJECT) {
                    if (request.headers.contains("range"_str)) {
                        (void)co_await response.close();
                        data.clear();
                        response = rstd_co_try(co_await execute(
                            prepared.clone(), downstream_request(base, request.headers)));
                        co_return Ok(passthrough(rstd::move(response),
                                                 rstd::move(permit),
                                                 prepared.clone(),
                                                 Some(rstd::move(buffer)),
                                                 Some(rstd::move(transfer))));
                    }
                    Vec<Bytes> prefix;
                    prefix.push(data.freeze());
                    prefix.push(rstd::move(chunk).unwrap());
                    response.effective_url = Some(rstd::move(effective));
                    auto delivery          = passthrough(rstd::move(response),
                                                         rstd::move(permit),
                                                         prepared.clone(),
                                                         Some(rstd::move(buffer)),
                                                         Some(rstd::move(transfer)),
                                                         rstd::move(prefix));
                    co_return Ok(rstd::move(delivery));
                }
                data.extend_from_slice(chunk->as_slice());
            }
            if (expected.is_some() && *expected != u64(data.len().to_primitive()))
                co_return Err(Error::state("incomplete object response"_str));
            if (! prepared->is_valid() || *stop.borrow()) co_return Err(Error::invalidated());
            auto group = rstd_co_try(
                licrypto::sha256_hex(request.scope.as_str().as_bytes()), [](auto error) {
                    return Error::state("resource group hashing failed"_str, rstd::move(error));
                });
            ObjectMetadata meta;
            meta.key           = key.clone();
            meta.group         = rstd::move(group);
            meta.provider      = request.provider;
            meta.length        = u64(data.len().to_primitive());
            meta.headers       = rstd::move(saved_headers);
            meta.policy        = rstd::move(policy);
            meta.target        = rstd_co_try(target_identity(*prepared, response));
            meta.effective_url = rstd_co_try(prepared->cache_url(effective.as_str()));
            Object completed { rstd::move(meta), data.freeze() };
            if (! bypass && completed.meta.policy.store) {
                if (store.is_some()) {
                    auto guard  = prepared->guard();
                    auto stored = co_await storage().put_object(completed.meta.clone(),
                                                                completed.bytes.clone(),
                                                                [guard = rstd::move(guard)] {
                                                                    return guard->is_valid();
                                                                });
                    if (stored.is_err())
                        rstd::log::warn("object cache write failed: {}",
                                        stored.as_ref().unwrap_err());
                    if (! prepared->is_valid()) (void)co_await storage().remove_object(key.clone());
                }
            } else if (store.is_some())
                (void)co_await storage().remove_object(key.clone());
            object = Some(rstd::move(completed));
            (void)co_await response.close();
        }
    }
    if (! prepared->is_valid()) co_return Err(Error::invalidated());
    auto completed = rstd::move(object).unwrap();
    auto effective = completed.meta.effective_url.clone();
    rstd_co_try(lihttpto::Url::parse(effective.as_str()), [](auto error) {
        return Error::state("invalid cached object effective URL"_str, rstd::move(error));
    });
    auto headers = restore(completed.meta.headers);
    if (not_modified(completed.meta.policy, request.headers)) {
        auto response          = empty_response(i32(304), rstd::move(headers));
        response.effective_url = Some(rstd::move(effective));
        co_return Ok(ResourceDelivery::Body(rstd::move(response)));
    }
    Metadata meta;
    meta.key        = rstd::move(completed.meta.key);
    meta.provider   = request.provider;
    meta.kind       = request.kind;
    meta.generation = rstd_co_try(resource_generation());
    meta.geometry   = rstd_co_try(Geometry::make(completed.meta.length));
    meta.headers    = rstd::move(completed.meta.headers);
    meta.policy     = rstd::move(completed.meta.policy);
    meta.target     = rstd::move(completed.meta.target);
    meta.ranges     = true;
    meta.layout     = Layout::Blocks();
    auto requested  = client_range(request.headers, meta);
    if (requested.is_none()) {
        (void)headers.set("content-range"_str,
                          rstd::format("bytes */{}", meta.geometry.length).as_str());
        auto response          = empty_response(i32(416), rstd::move(headers));
        response.effective_url = Some(rstd::move(effective));
        co_return Ok(ResourceDelivery::Body(rstd::move(response)));
    }
    auto range   = *requested;
    bool partial = range_requested(request.headers, meta);
    (void)headers.set("content-length"_str, rstd::format("{}", range.end - range.start).as_str());
    (void)headers.set("accept-ranges"_str, "bytes"_str);
    (void)headers.set("age"_str, rstd::format("{}", meta.policy.age()).as_str());
    if (partial)
        (void)headers.set(
            "content-range"_str,
            rstd::format("bytes {}-{}/{}", range.start, range.end - u64(1), meta.geometry.length)
                .as_str());
    auto body = Arc<ObjectBody>::make(
        rstd::move(completed.bytes), rstd::move(prepared), rstd::move(permit), rstd::move(buffer));
    auto stream = ObjectBody::stream(body.clone(), range);
    co_return Ok(
        ResourceDelivery::Body(Response { i32(partial ? 206 : 200),
                                          rstd::move(headers),
                                          resource_body(rstd::move(stream), rstd::move(body)),
                                          Some(rstd::move(effective)) }));
}
} // namespace qcm::backend::resource_cache
