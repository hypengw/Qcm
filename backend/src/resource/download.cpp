module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import :resource.service;
import :resource.service_state;
import :resource.store;
import :resource.download_key;
import rstd.log;
using namespace rstd::prelude;
using namespace rstd::literals;
using lihttpto::Headers;
using rstd::tuple;
using rstd::async::coro;
using rstd::async::Notify;
using rstd::async::Semaphore;
using rstd::bytes::Bytes;
using rstd::bytes::BytesMut;
using rstd::sync::Arc;
using rstd::sync::Weak;
using rstd::sync::atomic::Ordering;

namespace qcm::backend::resource_cache
{
auto ResourceCache::identity_encoding(ref<str> encoding) -> bool {
    constexpr auto expected = "identity"_str;
    if (encoding.len() != expected.len()) return false;
    for (usize index {}; index < encoding.len(); ++index) {
        auto byte = encoding.as_bytes()[index].to_primitive();
        if (byte >= 'A' && byte <= 'Z') byte += 'a' - 'A';
        if (byte != expected.as_bytes()[index].to_primitive()) return false;
    }
    return true;
}
auto ResourceCache::wait_download_cancelled(Arc<Download> download) -> coro<empty> {
    auto cancelled = download->cancel.subscribe();
    while (! *cancelled.borrow_and_update()) {
        auto changed = co_await cancelled.changed();
        if (changed.is_err()) break;
    }
    co_return empty {};
}
auto ResourceCache::download_closed(Arc<ResourceCache> cache, Arc<PreparedResource> prepared,
                                    Arc<Download> download) -> coro<empty> {
    (void)co_await rstd::async::select(
        cache->wait_stopped().into_future(),
        rstd::async::select(prepared->cancelled().into_future(),
                            wait_download_cancelled(rstd::move(download)).into_future()));
    co_return empty {};
}
auto ResourceCache::block(Version version, Arc<PreparedResource> prepared, Headers base, u64 index,
                          Option<Arc<Download>> initial, Arc<ReaderPosition> position, u64 window)
    -> coro<ServiceResult<BlockOutput>> {
    auto cached = rstd_co_try(co_await storage().read(version.clone(), index));
    if (cached.is_some())
        co_return Ok(BlockOutput { rstd::move(cached).unwrap(), rstd::move(initial) });
    if (initial.is_none()) {
        if (version.meta.policy.range_value().is_none()) {
            rstd_co_try(co_await storage().retire(version.clone()));
            co_return Err(Error::changed());
        }
        auto  start = index * version.meta.geometry.block;
        Range range = version.meta.ranges && version.meta.policy.range_value().is_some()
                          ? Range { start,
                                    rstd::cmp::min(start.saturating_add(window),
                                                   version.meta.geometry.length) }
                          : Range { u64(), version.meta.geometry.length };
        initial     = Some(rstd_co_try(co_await download(
            version.clone(), prepared.clone(), rstd::move(base), range, None(), position.clone())));
    }
    auto target   = rstd::move(initial).unwrap();
    auto progress = target->progress.subscribe();
    for (;;) {
        if (*state_->stop.borrow()) co_return Err(Error::invalidated());
        auto state = progress.borrow_and_update()->clone();
        {
            auto data  = target->state.lock().unwrap();
            auto found = data->blocks.get(index);
            if (found.is_some())
                co_return Ok(BlockOutput { (*found)->clone(), Some(target.clone()) });
        }
        auto bytes = rstd_co_try(co_await storage().read(version.clone(), index));
        if (bytes.is_some())
            co_return Ok(BlockOutput { rstd::move(bytes).unwrap(), Some(target.clone()) });
        RSTD_MATCH(rstd::move(state)) {
            RSTD_CASE(Running) {}
            RSTD_CASE(Complete) {
                co_return Err(Error::state("download did not produce requested block"_str));
            }
            RSTD_CASE(Failed, error) { co_return Err(rstd::move(error)); }
        }
        auto selected = co_await rstd::async::select(
            progress.changed(),
            download_closed(owner(), prepared.clone(), target.clone()).into_future());
        if (selected.is_right()) co_return Err(Error::invalidated());
        rstd_co_try(rstd::move(selected).unwrap_left(), [](auto cause) {
            return Error::state("resource progress channel closed"_str, rstd::move(cause));
        });
    }
}
auto ResourceCache::download(Version version, Arc<PreparedResource> prepared, Headers base,
                             Range range, Option<Response> response, Arc<ReaderPosition> position)
    -> coro<ServiceResult<Arc<Download>>> {
    auto                  active = co_await state_->downloads.lock();
    Option<Arc<Download>> found;
    auto                  cursor = position->cursor.load(Ordering::Acquire);
    active->retain([](const DownloadKey&, auto& target) {
        return target.strong_count() != usize();
    });
    for (auto [key, weak] : active->iter()) {
        if (key->version != version.id || cursor < key->start || cursor >= key->end) continue;
        auto target = weak->upgrade();
        if (! target) continue;
        found = Some(rstd::move(target));
        break;
    }
    if (found.is_some()) {
        {
            auto state = (*found)->state.lock().unwrap();
            state->readers.push(position.downgrade());
        }
        co_return Ok(rstd::move(found).unwrap());
    }
    if (response.is_none() && version.meta.ranges && version.meta.policy.range_value().is_some()) {
        range = rstd_co_try(co_await storage().missing_range(version.clone(), range));
        for (auto [key, weak] : active->iter()) {
            if (key->version == version.id && weak->strong_count() != usize() &&
                key->start > range.start && key->start < range.end)
                range.end = key->start;
        }
    }
    auto readable = rstd_co_try(Notify::make(), [](auto cause) {
        return Error::state("cannot create download notification"_str, rstd::move(cause));
    });
    auto target =
        Arc<Download>::make(version.meta.provider, version.id, range, rstd::move(readable));
    {
        auto state = target->state.lock().unwrap();
        state->readers.push(position.downgrade());
    }
    (void)active->insert(DownloadKey { version.id, range.start, range.end }, target.downgrade());
    {
        auto released = rstd::move(active);
    }
    // Task registration and shutdown share this lock; shutdown cannot miss a newly spawned task.
    auto pending = co_await state_->tasks.lock();
    if (*state_->stop.borrow()) {
        {
            auto released = rstd::move(pending);
        }
        (void)target->progress.send_replace(Progress::Failed(Error::invalidated()));
        co_return Err(Error::invalidated());
    }
    Vec<rstd::async::JoinHandle<empty>> running;
    for (auto& job : *pending) {
        if (! job.is_finished()) running.push(rstd::move(job));
    }
    auto job = rstd::async::spawn(run_download(owner(),
                                               target.clone(),
                                               rstd::move(version),
                                               rstd::move(prepared),
                                               rstd::move(base),
                                               range,
                                               rstd::move(response)));
    running.push(rstd::move(job));
    *pending = rstd::move(running);
    co_return Ok(rstd::move(target));
}
auto ResourceCache::run_download(Arc<ResourceCache> cache, Arc<Download> target, Version version,
                                 Arc<PreparedResource> prepared, Headers base, Range range,
                                 Option<Response> response) -> coro<empty> {
    auto result = co_await cache->fetch_blocks(target.clone(),
                                               rstd::move(version),
                                               rstd::move(prepared),
                                               rstd::move(base),
                                               range,
                                               rstd::move(response));
    (void)target->progress.send_replace(
        result.is_ok() ? Progress::Complete() : Progress::Failed(rstd::move(result).unwrap_err()));
    auto active = co_await cache->state_->downloads.lock();
    auto key    = DownloadKey { target->version, target->range.start, target->range.end };
    if (auto found = active->get(key)) {
        auto download = (*found)->upgrade();
        if (! download || Arc<Download>::ptr_eq(download, target)) (void)active->remove(key);
    }
    co_return empty {};
}
auto ResourceCache::flush_batch(Arc<Download> download, const Version& version,
                                Vec<tuple<u64, Bytes>>& batch) -> coro<bool> {
    Vec<u64> indexes;
    for (const auto& entry : batch) indexes.push(u64(rstd::get<0>(entry)));
    auto values = rstd::move(batch);
    batch       = {};
    auto result = co_await storage().write(version.clone(), rstd::move(values));
    if (result.is_err()) {
        rstd::log::warn("resource cache disabled for download: {}", result.as_ref().unwrap_err());
        co_return false;
    }
    {
        auto state = download->state.lock().unwrap();
        for (auto index : indexes) (void)state->blocks.remove(index);
    }
    (void)download->progress.send_replace(Progress::Running());
    co_return true;
}
auto ResourceCache::fetch_blocks(Arc<Download> target, Version version,
                                 Arc<PreparedResource> prepared, Headers headers, Range range,
                                 Option<Response> response) -> coro<ServiceResult<empty>> {
    Option<Arc<Semaphore>> resource_limit;
    {
        auto limits = co_await state_->per_resource.lock();
        limits->retain([](i64, auto& limit) {
            return limit.strong_count() != usize();
        });
        if (auto found = limits->get(version.id)) {
            auto limit = (*found)->upgrade();
            if (limit) resource_limit = Some(rstd::move(limit));
        }
        if (resource_limit.is_none()) {
            auto limit = Arc<Semaphore>::make(usize(2));
            (void)limits->insert(version.id, limit.downgrade());
            resource_limit = Some(rstd::move(limit));
        }
    }
    auto resource_acquired = co_await rstd::async::select(
        (*resource_limit)->clone().acquire_owned(),
        download_closed(owner(), prepared.clone(), target.clone()).into_future());
    if (resource_acquired.is_right()) co_return Err(Error::invalidated());
    auto resource_permit = rstd_co_try(rstd::move(resource_acquired).unwrap_left(), acquire_error);
    auto buffer   = rstd_co_try(co_await state_->buffers.clone().acquire_owned(), acquire_error);
    auto transfer = rstd_co_try(co_await state_->requests.clone().acquire_owned(), acquire_error);
    if (*target->cancel.borrow()) co_return Err(Error::invalidated());
    if (response.is_none()) {
        if (version.meta.ranges && version.meta.policy.range_value().is_some()) {
            (void)headers.set(
                "range"_str, rstd::format("bytes={}-{}", range.start, range.end - u64(1)).as_str());
            (void)headers.set("if-range"_str, *version.meta.policy.range_value());
        }
        auto fetched = co_await rstd::async::select(
            execute(prepared.clone(), rstd::move(headers)).into_future(),
            download_closed(owner(), prepared.clone(), target.clone()).into_future());
        if (fetched.is_right()) co_return Err(Error::invalidated());
        response = Some(rstd_co_try(rstd::move(fetched).unwrap_left()));
        if (response->status != i32(200) && response->status != i32(206)) {
            (void)co_await storage().retire(version.clone());
            co_return Err(Error::upstream(response->status, forwarded(response->headers)));
        }
        auto geometry = rstd_co_try(response_geometry(*response));
        if (geometry.is_none()) co_return Err(Error::state("resource length disappeared"_str));
        auto length   = rstd::get<0>(*geometry);
        auto actual   = rstd::get<1>(*geometry);
        auto identity = rstd_co_try(target_identity(*prepared, *response));
        auto encoding = value(response->headers, "content-encoding"_str);
        bool changed  = length != version.meta.geometry.length || identity != version.meta.target ||
                        (encoding.is_some() && ! identity_encoding(*encoding));
        if (version.meta.policy.range_value().is_some())
            changed = changed || ! version.meta.policy.matches_range(response->headers);
        else if (response->status != i32(200))
            co_return Err(Error::state("cannot combine unvalidated partial responses"_str));
        if (changed) {
            rstd_co_try(co_await storage().retire(version.clone()));
            co_return Err(Error::changed());
        }
        if (response->status == i32(206)) {
            if (actual.start != range.start || actual.end != range.end)
                co_return Err(Error::state("upstream returned wrong range"_str));
        } else if (actual.start != range.start || actual.end != range.end) {
            if (version.meta.policy.range_value().is_none())
                co_return Err(Error::state("cannot resume unvalidated representation"_str));
            rstd_co_try(co_await storage().retire(version.clone()));
            co_return Err(Error::changed());
        }
    }
    bool persist = Policy::from_response(response->headers, now(), now()).store;
    if (! persist) rstd_co_try(co_await storage().retire(version.clone()));
    BytesMut               buffered;
    u64                    cursor = range.start;
    Vec<tuple<u64, Bytes>> batch;
    usize                  batch_bytes {};
    auto                   last_commit = rstd::time::Instant::now();
    u64                    window_end  = range.start;
    Option<Window>         write_window;
    // Keep one pending read alive across wakeups and timed commits; canceling it may cancel HTTP
    // I/O.
    auto transfer_loop = [&]() -> coro<ServiceResult<empty>> {
        for (;;) {
            if (! version.is_valid() || ! prepared->is_valid()) co_return Err(Error::invalidated());
            bool has_readers = false;
            {
                auto state = target->state.lock().unwrap();
                for (const auto& weak : state->readers)
                    if (weak.strong_count() != usize()) has_readers = true;
            }
            if (! has_readers) co_return Ok(empty {});
            auto pending = response->next().into_future();
            auto read_deadline =
                rstd::time::Instant::now() + rstd::time::Duration::from_secs(u64(30));
            Option<Bytes> chunk;
            for (;;) {
                auto instant = rstd::time::Instant::now();
                if (instant >= read_deadline)
                    co_return Err(Error::request(lifetchto::Error::Timeout()));
                auto remaining    = read_deadline - instant;
                auto commit_delay = remaining;
                if (! batch.is_empty()) {
                    auto deadline = last_commit + rstd::time::Duration::from_millis(u64(250));
                    auto current  = rstd::time::Instant::now();
                    commit_delay =
                        deadline > current ? deadline - current : rstd::time::Duration {};
                }
                auto selected = co_await rstd::async::select(
                    rstd::async::timeout(rstd::future::by_ref(pending), remaining),
                    rstd::async::select(
                        download_closed(owner(), prepared.clone(), target.clone()).into_future(),
                        rstd::async::select(target->readable.notified(),
                                            rstd::async::sleep(commit_delay))));
                if (selected.is_left()) {
                    auto timed = rstd_co_try(rstd::move(selected).unwrap_left(),
                                             Error::request(lifetchto::Error::Timeout()));
                    chunk      = rstd_co_try(timed);
                    break;
                }
                auto event = rstd::move(selected).unwrap_right();
                if (event.is_left()) co_return Err(Error::invalidated());
                bool alive = false;
                {
                    auto state = target->state.lock().unwrap();
                    for (const auto& weak : state->readers)
                        if (weak.strong_count() != usize()) alive = true;
                }
                if (! alive) co_return Ok(empty {});
                if (! batch.is_empty() &&
                    last_commit.elapsed() >= rstd::time::Duration::from_millis(u64(250))) {
                    persist     = co_await flush_batch(target.clone(), version, batch);
                    batch_bytes = usize();
                    last_commit = rstd::time::Instant::now();
                }
            }
            if (chunk.is_none()) {
                if (cursor != range.end || ! buffered.is_empty())
                    co_return Err(Error::state("truncated resource body"_str));
                co_return Ok(empty {});
            }
            auto bytes = rstd::move(chunk).unwrap();
            if (u64(buffered.len().to_primitive()) > range.end - cursor ||
                u64(bytes.len().to_primitive()) >
                    range.end - cursor - u64(buffered.len().to_primitive()))
                co_return Err(Error::state("resource body exceeds declared range"_str));
            usize consumed {};
            while (consumed < bytes.len()) {
                if (cursor >= window_end) {
                    window_end   = rstd::cmp::min(range.end, cursor.saturating_add(FETCH_BYTES));
                    write_window = Some(rstd_co_try(
                        co_await storage().window(version.clone(), Range { cursor, window_end })));
                }
                auto index       = cursor / version.meta.geometry.block;
                auto block_range = rstd_co_try(version.meta.geometry.range(index));
                if (cursor != block_range.start)
                    co_return Err(Error::state("unaligned upstream range"_str));
                auto expected = usize((block_range.end - block_range.start).to_primitive());
                auto take     = rstd::cmp::min(expected - buffered.len(), bytes.len() - consumed);
                auto part     = bytes.slice(consumed, consumed + take);
                buffered.extend_from_slice(part.as_slice());
                consumed += take;
                if (buffered.len() != expected) continue;
                auto complete = buffered.split().freeze();
                if (! persist) {
                    for (;;) {
                        Option<u64> minimum;
                        usize       resident {};
                        {
                            auto state = target->state.lock().unwrap();
                            for (const auto& weak : state->readers) {
                                auto reader = weak.upgrade();
                                if (! reader) continue;
                                auto position = reader->cursor.load(Ordering::Acquire);
                                minimum =
                                    Some(minimum.is_some() ? rstd::cmp::min(*minimum, position)
                                                           : position);
                            }
                            if (minimum.is_none()) co_return Ok(empty {});
                            state->blocks.retain([&](const u64& i, Bytes&) {
                                return (i + u64(1)) * version.meta.geometry.block > *minimum;
                            });
                            for (auto value : state->blocks.values()) resident += value->len();
                        }
                        if (resident + complete.len() <= usize(FETCH_BYTES.to_primitive())) break;
                        auto wait = co_await rstd::async::select(
                            target->readable.notified(),
                            rstd::async::select(
                                rstd::async::sleep(rstd::time::Duration::from_millis(u64(100))),
                                download_closed(owner(), prepared.clone(), target.clone())
                                    .into_future()));
                        if (wait.is_right() && rstd::move(wait).unwrap_right().is_right())
                            co_return Err(Error::invalidated());
                    }
                }
                {
                    auto state = target->state.lock().unwrap();
                    (void)state->blocks.insert(index, complete.clone());
                }
                (void)target->progress.send_replace(Progress::Running());
                cursor = block_range.end;
                if (persist) {
                    batch_bytes += complete.len();
                    batch.push({ index, rstd::move(complete) });
                }
                if (persist && (batch_bytes >= usize(1024 * 1024) || cursor == range.end)) {
                    persist     = co_await flush_batch(target.clone(), version, batch);
                    batch_bytes = usize();
                    last_commit = rstd::time::Instant::now();
                }
            }
        }
    };
    auto result  = co_await transfer_loop();
    write_window = None();
    if (! batch.is_empty()) (void)co_await flush_batch(target.clone(), version, batch);
    (void)co_await response->close();
    co_return rstd::move(result);
}
} // namespace qcm::backend::resource_cache
