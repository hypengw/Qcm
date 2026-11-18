module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import :resource.service;
import :resource.store;
import :resource.response;
import liuuid;
import rstd.log;
using namespace rstd::prelude;
using namespace rstd::literals;
using lihttpto::Headers;
using rstd::tuple;
using rstd::async::coro;
using rstd::async::OwnedSemaphorePermit;
namespace watch = rstd::async::watch;
using rstd::bytes::Bytes;
using rstd::sync::Arc;

namespace qcm::backend::resource_cache
{
struct ResourceCache::UpstreamBody {
    Arc<ResourceCache>            cache;
    watch::Receiver<bool>         stopped;
    Option<Arc<PreparedResource>> prepared;
    Option<ResourceBody>          body;
    Option<OwnedSemaphorePermit>  reader;
    Option<OwnedSemaphorePermit>  buffer;
    Option<OwnedSemaphorePermit>  transfer;
    bool                          cancelled {};

    UpstreamBody(Arc<ResourceCache> cache, Option<Arc<PreparedResource>> prepared,
                 ResourceBody body, OwnedSemaphorePermit reader,
                 Option<OwnedSemaphorePermit> buffer, Option<OwnedSemaphorePermit> transfer)
        : cache(rstd::move(cache)),
          stopped(this->cache->stop.subscribe()),
          prepared(rstd::move(prepared)),
          body(Some(rstd::move(body))),
          reader(Some(rstd::move(reader))),
          buffer(rstd::move(buffer)),
          transfer(rstd::move(transfer)) {}
    ~UpstreamBody() { cancel(); }
    void cancel() {
        if (body.is_some()) body->cancel();
        cancelled = true;
        reader    = None();
        buffer    = None();
        transfer  = None();
    }
    auto close() -> coro<empty> {
        if (body.is_some()) (void)co_await body->close();
        body = None();
        cancel();
        co_return empty {};
    }
    static auto read_body(Arc<UpstreamBody> owner)
        -> coro<Option<tuple<ServiceResult<Bytes>, Arc<UpstreamBody>>>> {
        if (owner->body.is_none()) co_return None();
        auto result = co_await owner->body->next();
        if (result.is_err())
            co_return Some(tuple<ServiceResult<Bytes>, Arc<UpstreamBody>> {
                Err(rstd::move(result).unwrap_err()), rstd::move(owner) });
        if (result->is_none()) co_return None();
        co_return Some(tuple<ServiceResult<Bytes>, Arc<UpstreamBody>> {
            Ok(rstd::move(result).unwrap().unwrap()), rstd::move(owner) });
    }
    template<class Stream>
    struct Content {
        Arc<UpstreamBody> owner;
        Stream            stream;
    };
    template<class Stream>
    static auto advance(Option<Content<Stream>> state)
        -> coro<Option<tuple<ServiceResult<Bytes>, Option<Content<Stream>>>>> {
        if (state.is_none()) co_return None();
        auto owner = state->owner.clone();
        if (owner->cancelled) co_return None();
        auto invalidated = [&]() {
            return owner->prepared.is_some() &&
                   (! (*owner->prepared)->is_valid() || *owner->stopped.borrow());
        };
        Option<ServiceResult<Bytes>> item;
        if (invalidated()) {
            item = Some(ServiceResult<Bytes>(Err(Error::invalidated())));
        } else if (owner->prepared.is_none()) {
            item = co_await rstd::stream::next(state->stream);
        } else {
            auto selected = co_await rstd::async::select(
                rstd::stream::next(state->stream),
                rstd::async::select((*owner->prepared)->cancelled().into_future(),
                                    owner->stopped.changed()));
            if (selected.is_right() || invalidated())
                item = Some(ServiceResult<Bytes>(Err(Error::invalidated())));
            else
                item = rstd::move(selected).unwrap_left();
        }
        if (item.is_none() || item->is_err()) {
            // Drop the chained read future before closing the body it borrows.
            state = None();
            co_await owner->close();
        }
        if (item.is_none()) co_return None();
        co_return Some(tuple<ServiceResult<Bytes>, Option<Content<Stream>>> {
            rstd::move(item).unwrap(), rstd::move(state) });
    }
    static auto stream(Arc<UpstreamBody> owner, Vec<Bytes> prefix) {
        auto head    = rstd::stream::map(rstd::stream::iter(rstd::move(prefix)),
                                         [](Bytes bytes) -> ServiceResult<Bytes> {
                                          return Ok(rstd::move(bytes));
                                         });
        auto tail    = rstd::stream::unfold(owner.clone(), [](Arc<UpstreamBody> state) {
            return read_body(rstd::move(state)).into_future();
        });
        auto content = rstd::stream::chain(rstd::move(head), rstd::move(tail));
        using State  = Option<Content<decltype(content)>>;
        return rstd::stream::unfold(
            State(Some(Content<decltype(content)> { rstd::move(owner), rstd::move(content) })),
            [](State state) {
                return advance(rstd::move(state)).into_future();
            });
    }
};
auto ResourceCache::passthrough(Response response, OwnedSemaphorePermit reader,
                                Arc<PreparedResource> prepared, Option<OwnedSemaphorePermit> buffer,
                                Option<OwnedSemaphorePermit> transfer, Vec<Bytes> prefix)
    -> ResourceDelivery {
    response.headers = forwarded(response.headers);
    auto body        = Arc<UpstreamBody>::make(owner(),
                                               Some(rstd::move(prepared)),
                                               rstd::move(response.body),
                                               rstd::move(reader),
                                               rstd::move(buffer),
                                               rstd::move(transfer));
    auto stream      = UpstreamBody::stream(body.clone(), rstd::move(prefix));
    response.body    = resource_body(rstd::move(stream), rstd::move(body));
    return ResourceDelivery::Body(rstd::move(response));
}
auto ResourceCache::forward(Response response) -> coro<ServiceResult<ResourceDelivery>> {
    auto reader      = rstd_co_try(co_await readers.clone().acquire_owned(), acquire_error);
    response.headers = forwarded(response.headers);
    auto body        = Arc<UpstreamBody>::make(owner(),
                                               None<Arc<PreparedResource>>(),
                                               rstd::move(response.body),
                                               rstd::move(reader),
                                               None<OwnedSemaphorePermit>(),
                                               None<OwnedSemaphorePermit>());
    auto stream      = UpstreamBody::stream(body.clone(), Vec<Bytes> {});
    response.body    = resource_body(rstd::move(stream), rstd::move(body));
    co_return Ok(ResourceDelivery::Body(rstd::move(response)));
}

struct ResourceCache::Reader {
    Arc<ResourceCache>           cache;
    Version                      version;
    Arc<PreparedResource>        prepared;
    Headers                      base;
    Option<tuple<u64, Bytes>>    block;
    Option<Arc<Download>>        initial;
    Option<OwnedSemaphorePermit> permit;
    Option<Arc<ReaderPosition>>  position;
    bool                         only_cached;
    u64                          window;
    u32                          sequential {};
    Option<Window>               pin;
    Reader(Arc<ResourceCache> cache, Version version, Arc<PreparedResource> prepared, Headers base,
           Range range, Option<tuple<u64, Bytes>> block, Option<Arc<Download>> initial,
           OwnedSemaphorePermit permit, Arc<ReaderPosition> position, bool only_cached, Window pin)
        : cache(rstd::move(cache)),
          version(rstd::move(version)),
          prepared(rstd::move(prepared)),
          base(rstd::move(base)),
          block(rstd::move(block)),
          initial(rstd::move(initial)),
          permit(Some(rstd::move(permit))),
          position(Some(rstd::move(position))),
          only_cached(only_cached),
          window(range.start == u64() ? FETCH_BYTES : MAX_BLOCK),
          pin(Some(rstd::move(pin))) {}
    ~Reader() { cancel(); }
    void cancel() {
        // ResourceBody owns the source until its active read future has been destroyed.
        position = None();
        if (initial.is_some()) (void)(*initial)->readable.notifier().notify();
        initial = None();
        block   = None();
        pin     = None();
        permit  = None();
    }
    auto close() -> coro<empty> {
        cancel();
        co_return empty {};
    }
    auto read(Range range) -> coro<ServiceResult<Option<Bytes>>> {
        auto cursor = range.start;
        auto end    = range.end;
        if (permit.is_none()) co_return Ok(None());
        if (*cache->stop.borrow() || ! prepared->is_valid() || ! version.is_valid()) {
            co_return Err(Error::invalidated());
        }
        if (cursor == end) {
            co_return Ok(None());
        }
        auto index = cursor / version.meta.geometry.block;
        if (initial.is_some() &&
            (cursor < (*initial)->range.start || cursor >= (*initial)->range.end))
            initial = None();
        if (block.is_none() || rstd::get<0>(*block) != index) {
            auto start    = index * version.meta.geometry.block;
            auto next_pin = rstd_co_try(co_await cache->storage().window(
                version.clone(),
                Range { start, rstd::cmp::min(end, start.saturating_add(window)) }));
            pin           = Some(rstd::move(next_pin));
            if (block.is_some() && rstd::get<0>(*block) + u64(1) == index) {
                ++sequential;
                if (sequential >= u32(4)) {
                    window     = rstd::cmp::min(window * u64(2), FETCH_BYTES);
                    sequential = u32();
                }
            }
            if (only_cached) {
                auto bytes = rstd_co_try(co_await cache->storage().read(version.clone(), index));
                if (bytes.is_none()) {
                    co_return Err(Error::state("cached block unavailable"_str));
                }
                block   = Some(tuple<u64, Bytes>(index, rstd::move(bytes).unwrap()));
                initial = None();
            } else {
                Option<Arc<Download>> download;
                if (initial.is_some()) download = Some(initial->clone());
                auto read = rstd_co_try(co_await cache->block(version.clone(),
                                                              prepared.clone(),
                                                              base.clone(),
                                                              index,
                                                              rstd::move(download),
                                                              position->clone(),
                                                              window));
                block     = Some(tuple<u64, Bytes>(index, rstd::move(read.bytes)));
                initial   = rstd::move(read.download);
            }
        }
        const auto& bytes  = rstd::get<1>(*block);
        auto        offset = usize((cursor % version.meta.geometry.block).to_primitive());
        if (offset >= bytes.len()) {
            co_return Err(Error::state("cached block is truncated"_str));
        }
        auto length = rstd::cmp::min(
            OUTPUT_BYTES,
            rstd::cmp::min(bytes.len() - offset, usize((end - cursor).to_primitive())));
        auto chunk = bytes.slice(offset, offset + length);
        cursor += u64(length.to_primitive());
        (*position)->cursor.store(cursor, rstd::sync::atomic::Ordering::Release);
        if (initial.is_some()) (void)(*initial)->readable.notifier().notify();
        co_return Ok(Some(rstd::move(chunk)));
    }
    using State = tuple<Arc<Reader>, Range>;
    static auto advance(Option<State> state)
        -> coro<Option<tuple<ServiceResult<Bytes>, Option<State>>>> {
        if (state.is_none()) co_return None();
        auto& [owner, range] = *state;
        auto result          = co_await owner->read(range);
        if (result.is_err())
            co_return Some(tuple<ServiceResult<Bytes>, Option<State>> {
                Err(rstd::move(result).unwrap_err()), None() });
        if (result->is_none()) co_return None();
        auto bytes = rstd::move(result).unwrap().unwrap();
        range.start += u64(bytes.len().to_primitive());
        co_return Some(tuple<ServiceResult<Bytes>, Option<State>> { Ok(rstd::move(bytes)),
                                                                    rstd::move(state) });
    }
    static auto stream(Arc<Reader> owner, Range range) {
        return rstd::stream::unfold(Option<State>(Some(State { rstd::move(owner), range })),
                                    [](Option<State> state) {
                                        return advance(rstd::move(state)).into_future();
                                    });
    }
};

auto ResourceCache::resource_generation() -> ServiceResult<String> {
    auto value = rstd_try(liuuid::Uuid::new_v4(), Error::io(rstd::move(_e)));
    return Ok(value.simple());
}

auto ResourceCache::respond(ResourceRequest request) -> coro<ServiceResult<ResourceDelivery>> {
    if (*stop.borrow()) co_return Err(Error::invalidated());
    auto permit   = rstd_co_try(co_await readers.clone().acquire_owned(), acquire_error);
    auto prepared = request.prepared.clone();
    if (! prepared->is_valid()) co_return Err(Error::invalidated());
    auto base = request_headers(prepared->headers(), request.headers);
    if (store.is_none()) {
        if (only_cached(request.headers))
            co_return Ok(ResourceDelivery::Body(empty_response(i32(504))));
        auto response = rstd_co_try(
            co_await fetch(prepared.clone(), downstream_request(base, request.headers)));
        co_return Ok(passthrough(rstd::move(response), rstd::move(permit), rstd::move(prepared)));
    }
    auto            key     = rstd_co_try(request_identity(request, base));
    auto            opening = co_await open_lock(key.as_str());
    auto            opened  = co_await opening->lock();
    bool            bypass  = request_no_store(request.headers);
    Option<Version> cached;
    if (! bypass) cached = rstd_co_try(co_await storage().lookup(key.clone()));
    if (cached.is_some()) {
        bool downloading = false;
        {
            auto active = co_await downloads.lock();
            for (auto [key, target] : active->iter())
                if (key->version == cached->id && target->strong_count() != usize())
                    downloading = true;
        }
        if ((! downloading || request.headers.contains("range"_str)) &&
            (cached->meta.policy.range_value().is_none() || ! cached->meta.ranges) &&
            ! rstd_co_try(co_await storage().complete(cached->clone())))
            cached = None();
    }
    if (only_cached(request.headers) &&
        (cached.is_none() || ! cached->meta.policy.fresh(request.headers)))
        co_return Ok(ResourceDelivery::Body(empty_response(i32(504))));
    if (bypass || (request.headers.contains("if-range"_str) && cached.is_none())) {
        auto response = rstd_co_try(
            co_await fetch(prepared.clone(), downstream_request(base, request.headers)));
        co_return Ok(passthrough(rstd::move(response), rstd::move(permit), rstd::move(prepared)));
    }
    Option<Response> initial_response;
    Range            response_range { u64(), u64() };
    Option<Version>  chosen;
    if (cached.is_some() && cached->meta.policy.fresh(request.headers))
        chosen = Some(cached->clone());
    else {
        auto upstream_headers = base.clone();
        if (cached.is_some()) {
            auto conditions = cached->meta.policy.conditional();
            for (const auto& field : conditions.as_slice())
                (void)upstream_headers.set(field.name.as_str(), field.value.clone());
        }
        if (request.kind == ResourceKind::Audio) {
            u64 start {};
            if (! request.headers.contains("if-range"_str)) {
                auto range = value(request.headers, "range"_str);
                if (range.is_some() && range->starts_with("bytes="_str)) {
                    auto pair = range->get(usize(6), range->len()).unwrap().split_once("-"_str);
                    if (pair.is_some()) {
                        auto parsed = rstd::from_str<u64>(rstd::get<0>(*pair));
                        if (parsed.is_ok()) start = *parsed;
                    }
                }
            }
            start       = start / MAX_BLOCK * MAX_BLOCK;
            auto length = start == u64() ? FETCH_BYTES : MAX_BLOCK;
            (void)upstream_headers.set(
                "range"_str,
                rstd::format("bytes={}-{}", start, start.saturating_add(length - u64(1))).as_str());
        }
        auto sent     = now();
        auto response = rstd_co_try(co_await fetch(prepared.clone(), rstd::move(upstream_headers)));
        auto received = now();
        if (response.status == i32(304)) {
            if (cached.is_none()) co_return Err(Error::state("unexpected upstream 304"_str));
            rstd_co_try(ensure_validation(*prepared, cached->meta, response));
            auto headers                = merge_validation(cached->meta.headers, response.headers);
            auto meta                   = cached->meta.clone();
            meta.policy                 = Policy::from_response(headers, sent, received);
            meta.policy.range_validator = cached->meta.policy.range_validator.clone();
            meta.headers                = saved(headers);
            if (! meta.policy.store) {
                rstd_co_try(co_await storage().retire(cached->clone()));
                (void)co_await response.close();
                auto replacement = rstd_co_try(
                    co_await fetch(prepared.clone(), downstream_request(base, request.headers)));
                co_return Ok(
                    passthrough(rstd::move(replacement), rstd::move(permit), rstd::move(prepared)));
            }
            rstd_co_try(co_await storage().update(cached->clone(), meta.clone()));
            auto version = rstd::move(cached).unwrap();
            version.meta = rstd::move(meta);
            chosen       = Some(rstd::move(version));
        } else {
            if (response.status != i32(200) && response.status != i32(206))
                co_return Ok(
                    passthrough(rstd::move(response), rstd::move(permit), rstd::move(prepared)));
            auto policy = Policy::from_response(response.headers, sent, received);
            if (response.status == i32(206) && policy.range_value().is_none()) {
                (void)co_await response.close();
                sent     = now();
                response = rstd_co_try(
                    co_await fetch(prepared.clone(), downstream_request(base, request.headers)));
                policy = Policy::from_response(response.headers, sent, now());
                if (request.headers.contains("range"_str) || response.status != i32(200))
                    co_return Ok(passthrough(
                        rstd::move(response), rstd::move(permit), rstd::move(prepared)));
            }
            auto geometry = rstd_co_try(response_geometry(response));
            if (! policy.store || geometry.is_none()) {
                if (response.status == i32(206)) {
                    (void)co_await response.close();
                    response = rstd_co_try(co_await fetch(
                        prepared.clone(), downstream_request(base, request.headers)));
                }
                co_return Ok(
                    passthrough(rstd::move(response), rstd::move(permit), rstd::move(prepared)));
            }
            if (response.status == i32(200) && request.headers.contains("range"_str))
                co_return Ok(
                    passthrough(rstd::move(response), rstd::move(permit), rstd::move(prepared)));
            response_range = rstd::get<1>(*geometry);
            Metadata meta;
            meta.key        = key.clone();
            meta.provider   = request.provider;
            meta.kind       = request.kind;
            meta.generation = rstd_co_try(resource_generation());
            meta.geometry   = rstd_co_try(Geometry::make(rstd::get<0>(*geometry)));
            meta.headers    = saved(response.headers);
            meta.policy     = rstd::move(policy);
            meta.target     = rstd_co_try(target_identity(*prepared, response));
            meta.ranges     = response.status == i32(206);
            meta.layout     = Layout::Blocks();
            auto created    = co_await storage().create(rstd::move(meta));
            if (created.is_err()) {
                rstd::log::warn("resource cache unavailable: {}", created.as_ref().unwrap_err());
                (void)co_await response.close();
                auto replacement = rstd_co_try(
                    co_await fetch(prepared.clone(), downstream_request(base, request.headers)));
                co_return Ok(
                    passthrough(rstd::move(replacement), rstd::move(permit), rstd::move(prepared)));
            }
            chosen           = Some(rstd::move(created).unwrap());
            initial_response = Some(rstd::move(response));
        }
    }
    auto version = rstd::move(chosen).unwrap();
    if (not_modified(version.meta.policy, request.headers))
        co_return Ok(
            ResourceDelivery::Body(empty_response(i32(304), restore(version.meta.headers))));
    auto requested = client_range(request.headers, version.meta);
    if (requested.is_none()) {
        auto headers = restore(version.meta.headers);
        (void)headers.set("content-range"_str,
                          rstd::format("bytes */{}", version.meta.geometry.length).as_str());
        co_return Ok(ResourceDelivery::Body(empty_response(i32(416), rstd::move(headers))));
    }
    auto range = *requested;
    if (request.kind == ResourceKind::Image && ! request.headers.contains("range"_str)) {
        auto client = file_client_for(request.headers);
        if (client.is_some()) {
            auto local = co_await storage().local_file(version.clone(), rstd::move(*client));
            if (local.is_err() && local.as_ref().unwrap_err().is_PlainCorrupt())
                co_return Err(rstd::move(local).unwrap_err());
            if (local.is_err())
                rstd::log::warn("local cache file unavailable: {}", local.as_ref().unwrap_err());
            if (local.is_ok() && local->is_some()) {
                if (! version.is_valid() || ! prepared->is_valid())
                    co_return Err(Error::invalidated());
                co_return Ok(ResourceDelivery::LocalFile(rstd::move(*local).unwrap()));
            }
        }
    }
    auto position    = Arc<ReaderPosition>::make(range.start);
    auto pin         = rstd_co_try(co_await storage().window(
        version.clone(),
        Range { range.start, rstd::cmp::min(range.end, range.start.saturating_add(FETCH_BYTES)) }));
    bool cached_only = only_cached(request.headers);
    if (cached_only && ! rstd_co_try(co_await storage().covered(version.clone(), range)))
        co_return Ok(ResourceDelivery::Body(empty_response(i32(504))));
    Option<Arc<Download>> initial;
    if (initial_response.is_some()) {
        if (range.start >= response_range.start && range.start < response_range.end)
            initial = Some(rstd_co_try(co_await download(version.clone(),
                                                         prepared.clone(),
                                                         base.clone(),
                                                         response_range,
                                                         rstd::move(initial_response),
                                                         position.clone())));
    } else {
        auto active = co_await downloads.lock();
        for (auto [key, weak] : active->iter()) {
            if (key->version != version.id || range.start < key->start || range.start >= key->end)
                continue;
            auto target = weak->upgrade();
            if (! target) continue;
            {
                auto state = target->state.lock().unwrap();
                state->readers.push(position.downgrade());
            }
            initial = Some(rstd::move(target));
            break;
        }
    }
    auto headers = restore(version.meta.headers);
    (void)headers.set("age"_str, rstd::format("{}", version.meta.policy.age()).as_str());
    (void)headers.set("content-length"_str, rstd::format("{}", range.end - range.start).as_str());
    if (version.meta.ranges || rstd_co_try(co_await storage().complete(version.clone())))
        (void)headers.set("accept-ranges"_str, "bytes"_str);
    bool partial = range.start != u64() || range.end != version.meta.geometry.length ||
                   range_requested(request.headers, version.meta);
    if (partial)
        (void)headers.set(
            "content-range"_str,
            rstd::format(
                "bytes {}-{}/{}", range.start, range.end - u64(1), version.meta.geometry.length)
                .as_str());
    Option<tuple<u64, Bytes>> first;
    if (range.start < range.end && ! cached_only) {
        auto read = co_await block(version.clone(),
                                   prepared.clone(),
                                   base.clone(),
                                   range.start / version.meta.geometry.block,
                                   rstd::move(initial),
                                   position.clone(),
                                   range.start == u64() ? FETCH_BYTES : MAX_BLOCK);
        if (read.is_err()) {
            auto error = rstd::move(read).unwrap_err();
            RSTD_MATCH(rstd::move(error)) {
                RSTD_CASE(Upstream, status, upstream_headers) {
                    (void)upstream_headers.remove("content-encoding"_str);
                    co_return Ok(ResourceDelivery::Body(
                        empty_response(status, rstd::move(upstream_headers))));
                }
                RSTD_CASE_DEFAULT { co_return Err(rstd::move(error)); }
            }
        }
        first = Some(
            tuple<u64, Bytes>(range.start / version.meta.geometry.block, rstd::move(read->bytes)));
        initial = rstd::move(read->download);
    }
    if (! version.is_valid() || ! prepared->is_valid()) co_return Err(Error::invalidated());
    auto reader = Arc<Reader>::make(owner(),
                                    rstd::move(version),
                                    rstd::move(prepared),
                                    rstd::move(base),
                                    range,
                                    rstd::move(first),
                                    rstd::move(initial),
                                    rstd::move(permit),
                                    rstd::move(position),
                                    cached_only,
                                    rstd::move(pin));
    auto stream = Reader::stream(reader.clone(), range);
    co_return Ok(
        ResourceDelivery::Body(Response { i32(partial ? 206 : 200),
                                          rstd::move(headers),
                                          resource_body(rstd::move(stream), rstd::move(reader)),
                                          None() }));
}
} // namespace qcm::backend::resource_cache
