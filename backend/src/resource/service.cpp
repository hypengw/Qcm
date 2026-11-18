module;
#include <rstd/enum.hpp>
#include <rstd/macro.hpp>
module qcm.backend;
import :resource.service;
import :resource.service_state;
import :resource.store;
import :resource.policy;
import licrypto;
import liuuid;
import rstd.log;
import rstd.json;
using namespace rstd::prelude;
using namespace rstd::literals;
using lihttpto::Headers;
using rstd::tuple;
using rstd::async::AcquireError;
using rstd::async::coro;
using rstd::async::Notify;
using rstd::json::Value;
using rstd::sync::Arc;

namespace qcm::backend::resource_cache
{

auto ResourceCache::Progress::clone() const -> Progress {
    RSTD_MATCH(*this) {
        RSTD_CASE(Running) { return Running(); }
        RSTD_CASE(Complete) { return Complete(); }
        RSTD_CASE(Failed, error) { return Failed(error.clone()); }
    }
}

ResourceCache::Download::Download(i64 provider, i64 version, Range range, Notify readable)
    : provider(provider), version(version), range(range), readable(rstd::move(readable)) {}

ResourceCache::State::State(Option<Store> store): store(rstd::move(store)) {}

auto ResourceRequest::clone() const -> ResourceRequest {
    return { provider, kind, scope.clone(), prepared.clone(), headers.clone() };
}

ResourceCache::ResourceCache(OpenKey, Box<State> state): state_(rstd::move(state)) {}
ResourceCache::~ResourceCache() = default;

auto ResourceCache::owner() const -> Arc<ResourceCache> { return state_->self_.upgrade(); }

auto ResourceCache::storage() -> Store& { return *state_->store; }

auto ResourceCache::acquire_error(AcquireError error) -> Error {
    return Error::state("resource capacity acquisition failed"_str, rstd::move(error));
}

auto ResourceCache::open(rstd::path::PathBuf root) -> coro<ServiceResult<Arc<ResourceCache>>> {
    auto opened = co_await Store::open(root.join("resources-v1"_str));
    auto store  = [&]() -> Option<Store> {
        if (opened.is_ok()) return Some(rstd::move(opened).unwrap());
        rstd::log::warn("resource cache unavailable: {}", opened.as_ref().unwrap_err());
        return None();
    }();
    auto cache = Arc<ResourceCache>::make(OpenKey {}, Box<State>::make(rstd::move(store)));
    cache->state_->self_ = cache.downgrade();
    co_return Ok(rstd::move(cache));
}
auto ResourceCache::usage() -> coro<ServiceResult<Usage>> {
    if (state_->store.is_none()) co_return Ok(Usage {});
    co_return co_await storage().usage();
}
auto ResourceCache::wait_stopped() -> coro<empty> {
    auto stopped = state_->stop.subscribe();
    if (! *stopped.borrow()) (void)co_await stopped.changed();
    co_return empty {};
}
auto ResourceCache::invalidate(i64 provider) -> coro<ServiceResult<empty>> {
    Vec<Arc<Download>> invalidated;
    {
        auto active = co_await state_->downloads.lock();
        for (auto entry : active->values()) {
            auto download = entry->upgrade();
            if (download && download->provider == provider) invalidated.push(rstd::move(download));
        }
    }
    for (const auto& download : invalidated) {
        (void)download->cancel.send_replace(true);
        (void)download->progress.send_replace(Progress::Failed(Error::invalidated()));
    }
    if (state_->store.is_none()) co_return Ok(empty {});
    co_return co_await storage().invalidate(provider);
}
auto ResourceCache::shutdown() -> coro<ServiceResult<empty>> {
    if (state_->stop.send_replace(true)) co_return Ok(empty {});
    state_->readers.close();
    state_->buffers.close();
    state_->requests.close();
    auto pending  = co_await state_->tasks.lock();
    auto deadline = rstd::time::Instant::now() + rstd::time::Duration::from_secs(u64(5));
    for (auto& job : *pending) {
        auto instant = rstd::time::Instant::now();
        if (instant < deadline) {
            auto joined =
                co_await rstd::async::timeout(rstd::future::by_ref(job), deadline - instant);
            if (joined.is_ok()) continue;
        }
        job.abort();
        (void)co_await rstd::future::by_ref(job);
    }
    pending->clear();
    if (state_->store.is_none()) co_return Ok(empty {});
    co_return co_await storage().shutdown();
}
auto ResourceCache::open_lock(ref<str> key) -> coro<Arc<rstd::async::Mutex<empty>>> {
    auto entries = co_await state_->opens.lock();
    entries->retain([](const String&, auto& lock) {
        return lock.strong_count() != usize();
    });
    if (auto found = entries->get(key)) {
        auto lock = (*found)->upgrade();
        if (lock) co_return rstd::move(lock);
    }
    auto lock = Arc<rstd::async::Mutex<empty>>::make();
    (void)entries->insert(String::make(key), lock.downgrade());
    co_return rstd::move(lock);
}
auto ResourceCache::execute(Arc<PreparedResource> prepared, Headers headers)
    -> coro<ServiceResult<Response>> {
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (! prepared->is_valid() || *state_->stop.borrow()) co_return Err(Error::invalidated());
        auto selected = co_await rstd::async::select(prepared->fetch(headers.clone()).into_future(),
                                                     wait_stopped().into_future());
        if (selected.is_right()) co_return Err(Error::invalidated());
        auto response = rstd::move(selected).unwrap_left();
        if (response.is_ok()) co_return Ok(rstd::move(response).unwrap());
        auto error = rstd::move(response).unwrap_err();
        if (attempt != 0 || ! error.retryable_request()) co_return Err(rstd::move(error));
        auto delay = co_await rstd::async::select(
            rstd::async::sleep(rstd::time::Duration::from_millis(u64(250))),
            wait_stopped().into_future());
        if (delay.is_right()) co_return Err(Error::invalidated());
    }
    co_return Err(Error::state("resource fetch exhausted"_str));
}
auto ResourceCache::fetch(Arc<PreparedResource> prepared, Headers headers)
    -> coro<ServiceResult<Response>> {
    auto permit = rstd_co_try(co_await state_->requests.clone().acquire_owned(), acquire_error);
    co_return co_await execute(rstd::move(prepared), rstd::move(headers));
}
auto ResourceCache::delivery(ResourceRequest request) -> coro<ServiceResult<ResourceDelivery>> {
    if (request.prepared->cache_mode() != CacheMode::ByteRange)
        co_return co_await respond_object(rstd::move(request));
    auto response = co_await respond(request.clone());
    if (response.is_err()) {
        const auto& error = response.as_ref().unwrap_err();
        if (error.should_reopen()) response = co_await respond(rstd::move(request));
    }
    co_return rstd::move(response);
}
auto ResourceCache::object_response(ResourceRequest request)
    -> coro<ServiceResult<ResourceDelivery>> {
    co_return co_await respond_object(rstd::move(request));
}
auto ResourceCache::request_headers(const Headers& prepared, const Headers& client) -> Headers {
    auto                      headers           = prepared.clone();
    static constexpr ref<str> forwarded_names[] = {
        "accept"_str, "accept-language"_str, "icy-metadata"_str, "cache-control"_str, "pragma"_str
    };
    for (auto key : forwarded_names) {
        auto value = client.get(key);
        if (value.is_some()) (void)headers.set(key, (*value)->clone());
    }
    static constexpr ref<str> conditional_names[] = {
        "range"_str, "if-range"_str, "if-none-match"_str, "if-modified-since"_str
    };
    for (auto key : conditional_names) (void)headers.remove(key);
    return headers;
}
auto ResourceCache::downstream_request(const Headers& base, const Headers& client) -> Headers {
    auto                      headers = base.clone();
    static constexpr ref<str> names[] = {
        "range"_str, "if-range"_str, "if-none-match"_str, "if-modified-since"_str
    };
    for (auto key : names) {
        auto value = client.get(key);
        if (value.is_some()) (void)headers.set(key, (*value)->clone());
    }
    return headers;
}
auto ResourceCache::request_identity(const ResourceRequest& request, const Headers& base)
    -> ServiceResult<String> {
    auto url = request.prepared->cache_url(request.prepared->url());
    if (url.is_err()) return Err(rstd::move(url).unwrap_err());
    Vec<tuple<String, String>> canonical;
    for (const auto& field : base.as_slice()) {
        auto name = String::make(field.name.as_str());
        name.as_mut_str().make_ascii_lowercase();
        auto key = name.as_str();
        if (key == "cache-control"_str || key == "pragma"_str || key == "range"_str ||
            key == "if-range"_str || key == "if-none-match"_str || key == "if-modified-since"_str)
            continue;
        auto bytes = String::make("["_str);
        for (usize index {}; index < field.value.as_slice().len(); ++index) {
            if (index != usize()) bytes.push_str(", "_str);
            bytes.push_str(rstd::format("{}", u64(field.value[index].to_primitive())).as_str());
        }
        bytes.push_ascii(']');
        canonical.push({ rstd::move(name), rstd::move(bytes) });
    }
    rstd::slice_::sort_by(canonical.as_mut_slice().as_mut_ref(), [](const auto& a, const auto& b) {
        auto order = rstd::iter::from_slice(rstd::get<0>(a).as_str().as_bytes())
                         .copied()
                         .cmp(rstd::iter::from_slice(rstd::get<0>(b).as_str().as_bytes()).copied());
        return order != rstd::strong_ordering::equal
                   ? order
                   : rstd::iter::from_slice(rstd::get<1>(a).as_str().as_bytes())
                         .copied()
                         .cmp(rstd::iter::from_slice(rstd::get<1>(b).as_str().as_bytes()).copied());
    });
    Vec<Value> fields;
    for (auto& entry : canonical) {
        Vec<Value> pair;
        pair.push(Value::String(rstd::move(rstd::get<0>(entry))));
        pair.push(Value::String(rstd::move(rstd::get<1>(entry))));
        fields.push(Value::Array(rstd::move(pair)));
    }
    Vec<Value> identity;
    identity.push(Value::Number(rstd::json::Number::from_i64(request.provider)));
    identity.push(Value::String(request.scope.clone()));
    identity.push(Value::String(rstd::move(url).unwrap()));
    identity.push(Value::Array(rstd::move(fields)));
    auto encoded = rstd::json::to_string(Value::Array(rstd::move(identity)));
    auto digest  = licrypto::sha256_hex(encoded.as_str().as_bytes());
    if (digest.is_err())
        return Err(Error::state("resource identity failed"_str, rstd::move(digest).unwrap_err()));
    return Ok(rstd::move(digest).unwrap());
}
auto ResourceCache::target_identity(const PreparedResource& prepared, const Response& response)
    -> ServiceResult<String> {
    auto effective =
        response.effective_url.is_some() ? response.effective_url->as_str() : prepared.url();
    auto normalized = prepared.cache_url(effective);
    if (normalized.is_err()) return Err(rstd::move(normalized).unwrap_err());
    auto digest = licrypto::sha256_hex(normalized->as_str().as_bytes());
    if (digest.is_err())
        return Err(
            Error::state("resource target hashing failed"_str, rstd::move(digest).unwrap_err()));
    return Ok(rstd::move(digest).unwrap());
}
auto ResourceCache::response_geometry(const Response& response)
    -> ServiceResult<Option<tuple<u64, Range>>> {
    auto content_length = response.content_length();
    if (response.status != i32(206)) {
        if (content_length.is_none()) return Ok(None());
        auto length = *content_length;
        return Ok(Some(tuple<u64, Range>(length, Range { u64(), length })));
    }
    auto text = value(response.headers, "content-range"_str);
    if (text.is_none() || ! text->starts_with("bytes "_str))
        return Err(Error::state("invalid Content-Range"_str));
    auto full = text->get(usize(6), text->len()).unwrap().split_once("/"_str);
    if (full.is_none()) return Err(Error::state("invalid Content-Range"_str));
    auto pair = rstd::get<0>(*full).split_once("-"_str);
    if (pair.is_none()) return Err(Error::state("invalid Content-Range"_str));
    auto start = rstd::from_str<u64>(rstd::get<0>(*pair));
    auto end   = rstd::from_str<u64>(rstd::get<1>(*pair));
    auto total = rstd::from_str<u64>(rstd::get<1>(*full));
    if (start.is_err())
        return Err(
            Error::state("invalid response range start"_str, rstd::move(start).unwrap_err()));
    if (end.is_err())
        return Err(Error::state("invalid response range end"_str, rstd::move(end).unwrap_err()));
    if (total.is_err())
        return Err(
            Error::state("invalid response range length"_str, rstd::move(total).unwrap_err()));
    if (*start > *end || *end >= *total) return Err(Error::state("invalid response range"_str));
    auto length = *end - *start + u64(1);
    if (content_length.is_none()) return Ok(None());
    if (*content_length != length) return Err(Error::state("range length mismatch"_str));
    return Ok(Some(tuple<u64, Range>(*total, Range { *start, *end + u64(1) })));
}
auto ResourceCache::range_requested(const Headers& headers, const Metadata& meta) -> bool {
    auto if_range  = value(headers, "if-range"_str);
    auto validator = meta.policy.range_value();
    if (if_range.is_some() && (validator.is_none() || *validator != *if_range)) return false;
    auto text = value(headers, "range"_str);
    if (text.is_none() || ! text->starts_with("bytes="_str) || text->contains(","_str))
        return false;
    auto pair = text->get(usize(6), text->len()).unwrap().split_once("-"_str);
    if (pair.is_none()) return false;
    auto start = rstd::get<0>(*pair), end = rstd::get<1>(*pair);
    if (start.is_empty()) return rstd::from_str<u64>(end).is_ok();
    auto parsed = rstd::from_str<u64>(start);
    if (parsed.is_err()) return false;
    if (end.is_empty()) return true;
    auto last = rstd::from_str<u64>(end);
    return last.is_ok() && *last >= *parsed;
}
auto ResourceCache::client_range(const Headers& headers, const Metadata& meta) -> Option<Range> {
    auto length = meta.geometry.length;
    if (! range_requested(headers, meta)) return Some(Range { u64(), length });
    auto text  = value(headers, "range"_str).unwrap();
    auto pair  = text.get(usize(6), text.len()).unwrap().split_once("-"_str).unwrap();
    auto first = rstd::get<0>(pair), last = rstd::get<1>(pair);
    if (first.is_empty()) {
        auto suffix = rstd::from_str<u64>(last).unwrap();
        if (suffix == u64() || length == u64()) return None();
        return Some(Range { suffix >= length ? u64() : length - suffix, length });
    }
    auto start = rstd::from_str<u64>(first).unwrap();
    auto end =
        last.is_empty()
            ? length
            : rstd::cmp::min(rstd::from_str<u64>(last).unwrap().saturating_add(u64(1)), length);
    if (start >= end || start >= length) return None();
    return Some(Range { start, end });
}
auto ResourceCache::empty_response(i32 status, Headers headers) -> Response {
    if (status != i32(304)) (void)headers.set("content-length"_str, "0"_str);
    return { status, rstd::move(headers), {}, None() };
}
auto ResourceCache::ensure_validation(const PreparedResource& prepared, const Metadata& old,
                                      const Response& response) -> ServiceResult<empty> {
    auto length = value(response.headers, "content-length"_str);
    if (length.is_some()) {
        auto parsed = rstd::from_str<u64>(*length);
        if (parsed.is_err())
            return Err(Error::state("invalid 304 representation length"_str,
                                    rstd::move(parsed).unwrap_err()));
        if (*parsed != old.geometry.length)
            return Err(Error::state("304 representation length changed"_str));
    }
    auto target = rstd_try(target_identity(prepared, response));
    if (target != old.target) return Err(Error::state("validation target changed"_str));
    static constexpr ref<str> names[] = { "etag"_str, "last-modified"_str };
    for (auto name : names) {
        auto        actual   = value(response.headers, name);
        const auto& expected = name == "etag"_str ? old.policy.etag : old.policy.modified;
        if (actual.is_some() && (expected.is_none() || expected->as_str() != *actual))
            return Err(Error::state("304 validator changed"_str));
    }
    return Ok(empty {});
}

auto ResourceCache::authorize_file_client(ref<str> key) -> ServiceResult<empty> {
#ifndef __linux__
    return Err(Error::state("local file delivery is not verified on this platform"_str));
#else
    if (key.len() != usize(32)) return Err(Error::state("invalid file client key"_str));
    auto uuid = liuuid::Uuid::parse_str(key);
    if (uuid.is_err() || uuid->version() != u8(4) || uuid->variant() != liuuid::Variant::Rfc4122)
        return Err(Error::state("invalid file client key"_str));
    auto digest = licrypto::sha256_hex(key.as_bytes());
    if (digest.is_err())
        return Err(Error::state("file client hashing failed"_str, rstd::move(digest).unwrap_err()));
    auto client = state_->file_client.lock().unwrap();
    *client     = Some(rstd::move(digest).unwrap());
    return Ok(empty {});
#endif
}
auto ResourceCache::authenticated_file_client(const Headers& headers) const -> Option<String> {
    usize count {};
    for (const auto& field : headers.as_slice())
        if (field.name.matches("qcm-file-client"_str)) ++count;
    auto input = headers.get("qcm-file-client"_str);
    if (count != usize(1) || input.is_none() || (*input)->as_slice().len() != usize(32))
        return None();
    auto digest = licrypto::sha256_hex((*input)->as_slice());
    if (digest.is_err()) {
        rstd::log::warn("file client hashing failed: {}", digest.as_ref().unwrap_err());
        return None();
    }
    auto expected = state_->file_client.lock().unwrap();
    if (expected->is_none() || **expected != *digest) return None();
    return Some(rstd::move(digest).unwrap());
}
auto ResourceCache::file_client_for(const Headers& headers) const -> Option<String> {
    usize count {};
    for (const auto& field : headers.as_slice())
        if (field.name.matches("qextra-image-transport"_str)) ++count;
    auto transport = value(headers, "qextra-image-transport"_str);
    if (count != usize(1) || transport.is_none() || *transport != "file-lease-v1"_str)
        return None();
    return authenticated_file_client(headers);
}
auto ResourceCache::file_lease_response(Option<Arc<ResourceCache>> cache, ref<str> method,
                                        ref<str> path, const Headers& headers)
    -> coro<Option<Response>> {
    constexpr auto prefix = "/resource-file-leases/"_str;
    if (! path.starts_with(prefix)) co_return None();
    auto reply = [](int status) {
        auto result = empty_response(i32(status));
        (void)result.headers.set("cache-control"_str, "no-store"_str);
        return result;
    };
    auto client = cache.is_some() ? (*cache)->authenticated_file_client(headers) : None<String>();
    if (client.is_none()) co_return Some(reply(403));
    auto rest  = path.get(prefix.len(), path.len()).unwrap();
    auto pair  = rest.split_once("/"_str);
    bool renew = pair.is_some();
    auto token = renew ? rstd::get<0>(*pair) : rest;
    if (renew && rstd::get<1>(*pair) != "renew"_str) co_return Some(reply(404));
    if (token.len() != usize(32) || liuuid::Uuid::parse_str(token).is_err())
        co_return Some(reply(404));
    if ((*cache)->state_->store.is_none()) co_return Some(reply(503));
    int status = 405;
    if (method == "POST"_str && renew) {
        auto result =
            co_await (*cache)->storage().renew_file(String::make(token), rstd::move(*client));
        if (result.is_err())
            rstd::log::warn("file lease renewal failed: {}", result.as_ref().unwrap_err());
        status = result.is_err() ? 503 : *result ? 204 : 410;
    } else if (method == "DELETE"_str && ! renew) {
        auto result =
            co_await (*cache)->storage().release_file(String::make(token), rstd::move(*client));
        if (result.is_err())
            rstd::log::warn("file lease release failed: {}", result.as_ref().unwrap_err());
        status = result.is_err() ? 503 : 204;
    }
    auto result = reply(status);
    if (renew && status == 204)
        (void)result.headers.set("qcm-file-lease-seconds"_str,
                                 rstd::format("{}", LEASE_SECONDS.to_primitive()).as_str());
    co_return Some(rstd::move(result));
}
} // namespace qcm::backend::resource_cache
