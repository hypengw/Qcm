module;
#include <rstd/macro.hpp>
module qcm.backend;
import :resource.store;
import :resource.storage;
import :resource.schema;
import :resource.startup;
import licrypto;
import rstd.log;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::bytes::Bytes;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::Condvar;
using rstd::sync::Mutex;
using rstd::sync::atomic::Atomic;
using rstd::sync::atomic::Ordering;
using rstd::sync::mpmc::RecvTimeoutError;
using WorkerThread = rstd::thread::JoinHandle<empty>;

namespace qcm::backend::resource_cache::store_detail
{
namespace q = liorm::query;
using namespace cache_db;
inline constexpr auto retire_version = q::compile([](q::QueryBuilder& b) {
    auto v = b.use(schema).table<VersionRecord>();
    return b.update(v.source())
        .set(v.col<&VersionRecord::obsolete>().set(b.integer(1)))
        .where(v.col<&VersionRecord::id>() == b.param<i64>("id"_str));
});
inline constexpr auto delete_object  = q::compile([](q::QueryBuilder& b) {
    auto o = b.use(schema).table<ObjectRecord>();
    return b.delete_from(o.source())
        .where(o.col<&ObjectRecord::resource_key>() == b.param<q::TextParameter>("key"_str));
});
inline constexpr auto update_object  = q::compile([](q::QueryBuilder& b) {
    auto o = b.use(schema).table<ObjectRecord>();
    return b.update(o.source())
        .set(o.col<&ObjectRecord::meta>().set(b.param<q::TextParameter>("meta"_str)))
        .where(o.col<&ObjectRecord::resource_key>() == b.param<q::TextParameter>("key"_str));
});
inline constexpr auto release_lease  = q::compile([](q::QueryBuilder& b) {
    auto l = b.use(schema).table<FileLeaseRecord>();
    return b.delete_from(l.source())
        .where((l.col<&FileLeaseRecord::token>() == b.param<q::TextParameter>("token"_str)) &&
               (l.col<&FileLeaseRecord::client>() == b.param<q::TextParameter>("client"_str)));
});
struct Control {
    rstd::async::Notify space;
    Atomic<bool>        closed { false };
    explicit Control(rstd::async::Notify notify): space(rstd::move(notify)) {}
    Atomic<bool> stop { false };
    Mutex<bool>  pending { false };
    Condvar      wake;
    void         notify() {
        {
            auto guard = pending.lock().unwrap();
            *guard     = true;
        }
        wake.notify_one();
    }
};
using JobSender   = rstd::sync::mpmc::Sender<Job>;
using JobReceiver = rstd::sync::mpmc::Receiver<Job>;
template<class F>
auto blocking_call(const JobSender& sender, F function)
    -> decltype(function(rstd::declval<Storage&>())) {
    using Output = decltype(function(rstd::declval<Storage&>()));
    auto pair    = rstd::async::oneshot::channel<Output>();
    auto job     = make_job(rstd::move(function), rstd::move(pair.template get<0>()));
    if (sender.send(rstd::move(job)).is_err())
        return Err(Error::state("cache storage stopped"_str));
    auto result = rstd::async::block_on(rstd::move(pair.template get<1>()));
    if (result.is_err()) return Err(Error::state("cache storage stopped without result"_str));
    return rstd::move(*result);
}
struct Opened {
    PathBuf            root;
    rstd::fs::File     owner;
    sqlito::Connection connection;
};
struct ActorExit {
    Arc<Control> control;
    ~ActorExit() {
        control->closed.store(true, Ordering::Release);
        (void)control->space.notifier().notify();
    }
};
auto open_files(PathBuf root) -> DbResult<Opened> {
    auto created = rstd::fs::create_dir_all(root.as_path());
    if (created.is_err()) return Err(Error::io(rstd::move(created).unwrap_err()));
    auto info = rstd::fs::symlink_metadata(root.as_path());
    if (info.is_err()) return Err(Error::io(rstd::move(info).unwrap_err()));
    if (! info->is_dir() || info->is_symlink())
        return Err(Error::state("invalid cache directory"_str));
    rstd_try(private_directory(root.as_path()));
    auto canonical = rstd::fs::canonicalize(root.as_path());
    if (canonical.is_err()) return Err(Error::io(rstd::move(canonical).unwrap_err()));
    root        = rstd::move(*canonical);
    auto locked = rstd_try(open_owner(root.as_path()));
    for (auto filename :
         array<ref<str>, 3> { "meta.sqlite"_str, "meta.sqlite-wal"_str, "meta.sqlite-shm"_str }) {
        auto path     = root.join(filename);
        auto metadata = rstd::fs::symlink_metadata(path.as_path());
        if (metadata.is_ok() && (! metadata->is_file() || metadata->is_symlink()))
            return Err(Error::state("invalid cache database file"_str));
        if (metadata.is_err() && metadata.unwrap_err().kind().code != rstd::io::ErrorKind::NotFound)
            return Err(Error::io(rstd::move(metadata).unwrap_err()));
    }
    auto path = root.join("meta.sqlite"_str);
    auto name = path.as_path().to_str();
    if (name.is_none()) return Err(Error::state("invalid cache database path"_str));
    auto connection = rstd_try(sqlito::Connection::open(*name));
    return Ok(Opened { rstd::move(root), rstd::move(locked), rstd::move(connection) });
}
auto storage_worker(PathBuf root, JobReceiver receiver, Arc<Control> control,
                    rstd::async::oneshot::Sender<DbResult<empty>> ready) -> empty {
    ActorExit    exit { control.clone() };
    StartupTrace trace(STARTUP_LABELS);
    auto         result = [&]() -> DbResult<empty> {
        auto files   = rstd_try(trace.measure(StartupStage::Files, [&] {
            return open_files(rstd::move(root));
        }));
        auto storage = rstd_try(trace.measure(StartupStage::Storage, [&] {
            return Storage::open(
                files.connection, rstd::move(files.root), rstd::move(files.owner), trace);
        }));
        storage.db.attach(files.connection);
        trace.finish("ready"_str);
        (void)ready.send(Ok(empty {}));
        for (;;) {
            storage.cleanup_garbage();
            auto job = [&]() -> Result<Job, RecvTimeoutError> {
                if (auto deadline = storage.cleanup_deadline())
                    return receiver.recv_deadline(*deadline);
                return receiver.recv().map_err([](auto) {
                    return RecvTimeoutError::Disconnected;
                });
            }();
            if (job.is_err()) {
                if (job.unwrap_err() == RecvTimeoutError::Timeout) continue;
                break;
            }
            (void)control->space.notifier().notify();
            (*job)->operator()(storage);
            if (storage.stopped) break;
        }
        return Ok(empty {});
    }();
    if (result.is_err()) {
        trace.finish("failed"_str);
        (void)ready.send(Err(rstd::move(result).unwrap_err()));
    }
    return empty {};
}
auto copy_blocks(const JobSender& sender, Ticket& ticket, const Arc<Control>& control)
    -> DbResult<array<u8, 32>> {
    auto created = licrypto::Sha256::make();
    if (created.is_err())
        return Err(Error::state("file digest unavailable"_str, rstd::move(created).unwrap_err()));
    auto hasher = rstd::move(*created);
    u64  length {};
    for (u64 block {}; block < ticket.version.meta.geometry.count(); ++block) {
        if (control->stop.load(Ordering::Acquire))
            return Err(Error::state("file conversion cancelled"_str));
        auto data = rstd_try(
            blocking_call(sender, [id = ticket.version.id, block](Storage& s) -> DbResult<Bytes> {
                rstd_try(s.check_build(id));
                auto data = rstd_try(s.read(id, block));
                if (data.is_none()) return Err(Error::state("conversion block missing"_str));
                return Ok(rstd::move(*data));
            }));
        auto written = ticket.file.write_all_at(data.as_slice(), length);
        if (written.is_err()) return Err(Error::io(rstd::move(written).unwrap_err()));
        auto hashed = hasher.update(data.as_slice());
        if (hashed.is_err())
            return Err(
                Error::state("file conversion digest failed"_str, rstd::move(hashed).unwrap_err()));
        length += u64(data.len().to_primitive());
        auto bytes = rstd_try(allocated(ticket.file));
        rstd_try(blocking_call(sender, [id = ticket.version.id, bytes](Storage& s) {
            return s.copied_block(id, bytes);
        }));
    }
    if (length != ticket.version.meta.geometry.length)
        return Err(Error::state("file conversion length mismatch"_str));
    auto synced = ticket.file.sync_all();
    if (synced.is_err()) return Err(Error::io(rstd::move(synced).unwrap_err()));
    auto digest = rstd::move(hasher).finalize();
    if (digest.is_err())
        return Err(Error::state("file digest failed"_str, rstd::move(digest).unwrap_err()));
    auto checked = licrypto::Sha256::make();
    if (checked.is_err())
        return Err(Error::state("file verification digest unavailable"_str,
                                rstd::move(checked).unwrap_err()));
    auto buffer = Vec<u8>::make();
    buffer.resize(usize(rstd::cmp::min(ticket.version.meta.geometry.block, length).to_primitive()),
                  u8());
    u64 verified {};
    for (;;) {
        if (control->stop.load(Ordering::Acquire))
            return Err(Error::state("file verification cancelled"_str));
        auto count = ticket.file.read_at(buffer.as_mut_slice().as_mut_ref(), verified);
        if (count.is_err()) return Err(Error::io(rstd::move(count).unwrap_err()));
        if (*count == usize()) break;
        auto view   = slice<u8>::from_raw_parts(buffer.as_slice().as_raw_ptr(), *count);
        auto hashed = checked->update(view);
        if (hashed.is_err())
            return Err(
                Error::state("file verification hash failed"_str, rstd::move(hashed).unwrap_err()));
        verified += u64(count->to_primitive());
        if (verified > length) return Err(Error::state("file grew during verification"_str));
    }
    auto actual = rstd::move(*checked).finalize();
    if (actual.is_err())
        return Err(
            Error::state("file verification digest failed"_str, rstd::move(actual).unwrap_err()));
    if (verified != length || *actual != *digest)
        return Err(Error::state("file verification failed"_str));
    return Ok(*digest);
}
auto worker(JobSender sender, Arc<Control> control) -> empty {
    while (! control->stop.load(Ordering::Acquire)) {
        auto next = blocking_call(sender, [](Storage& s) {
            return s.next_build();
        });
        if (next.is_ok() && next->is_some()) {
            auto ticket  = rstd::move(**next);
            auto id      = ticket.version.id;
            auto digest  = copy_blocks(sender, ticket, control);
            auto version = ticket.version.clone();
            {
                auto closed = rstd::move(ticket.file);
            }
            auto published = [&]() -> DbResult<empty> {
                if (digest.is_err()) return Err(rstd::move(digest).unwrap_err());
                if (control->stop.load(Ordering::Acquire))
                    return Err(Error::state("file conversion cancelled"_str));
                return blocking_call(
                    sender, [version = rstd::move(version), digest = *digest](Storage& s) mutable {
                        return s.publish_file(rstd::move(version), digest);
                    });
            }();
            if (published.is_err()) {
                rstd::log::warn("cache file conversion deferred");
                (void)blocking_call(sender, [id](Storage& s) {
                    return s.finish_failed(id);
                });
            }
            continue;
        }
        if (next.is_err()) rstd::log::warn("cache file scheduler deferred");
        bool rescan {};
        {
            auto pending = control->pending.lock().unwrap();
            (void)control->wake.wait_timeout_while(
                pending, rstd::time::Duration::from_secs(u64(1)), [&](bool ready) {
                    return ! ready && ! control->stop.load(Ordering::Acquire);
                });
            rescan   = *pending;
            *pending = false;
        }
        if (rescan)
            (void)blocking_call(sender, [](Storage& s) -> DbResult<empty> {
                s.scan_cursor = i64();
                return Ok(empty {});
            });
    }
    return empty {};
}
struct Owner {
    rstd::async::Mutex<empty>   admission { empty {} };
    rstd::async::Mutex<empty>   shutdown { empty {} };
    Mutex<Option<JobSender>>    sender;
    Arc<Control>                control;
    Mutex<Option<WorkerThread>> materializer;
    Mutex<Option<WorkerThread>> storage;
    Owner(JobSender sender, Arc<Control> control)
        : sender(Some(rstd::move(sender))),
          control(rstd::move(control)),
          materializer(None()),
          storage(None()) {}
    auto sender_handle() -> DbResult<JobSender> {
        auto guard = sender.lock().unwrap();
        if (guard->is_none()) return Err(Error::state("cache storage stopped"_str));
        return Ok(JobSender(**guard));
    }
    auto stop_worker() -> DbResult<empty> {
        control->stop.store(true, Ordering::Release);
        control->notify();
        Option<WorkerThread> thread;
        {
            auto guard = materializer.lock().unwrap();
            thread     = guard->take();
        }
        if (thread.is_some() && rstd::move(*thread).join().is_err())
            return Err(Error::state("cache file worker failed to stop"_str));
        return Ok(empty {});
    }
    auto join_storage() -> DbResult<empty> {
        {
            auto guard = sender.lock().unwrap();
            *guard     = None();
        }
        Option<WorkerThread> thread;
        {
            auto guard = storage.lock().unwrap();
            thread     = guard->take();
        }
        if (thread.is_some() && rstd::move(*thread).join().is_err())
            return Err(Error::state("cache storage worker failed to stop"_str));
        return Ok(empty {});
    }
    ~Owner() {
        (void)stop_worker();
        (void)join_storage();
    }
};
} // namespace qcm::backend::resource_cache::store_detail

namespace qcm::backend::resource_cache
{
Store::Store(Arc<store_detail::Owner> owner): owner_(rstd::move(owner)) {}
Store::Store(Store&&) noexcept = default;
Store::~Store()                = default;

template<class T>
auto Store::submit(Arc<store_detail::Owner> owner, store_detail::Job job,
                   rstd::async::oneshot::Receiver<DbResult<T>> reply) -> StorageFuture<T> {
    {
        auto admitted = co_await owner->admission.lock();
        auto handle   = rstd_co_try(owner->sender_handle());
        for (;;) {
            if (owner->control->closed.load(Ordering::Acquire))
                co_return Err(Error::state("cache storage stopped"_str));
            auto sent = handle.try_send(rstd::move(job));
            if (sent.is_ok()) break;
            auto error = rstd::move(sent).unwrap_err();
            if (error.is_disconnected()) co_return Err(Error::state("cache storage stopped"_str));
            job = rstd::move(error).into_inner();
            rstd_co_try(co_await owner->control->space.notified(), Error::io);
        }
    }
    co_return rstd_co_try(co_await rstd::move(reply),
                          Error::state("cache storage stopped without result"_str));
}

template<class F>
auto Store::call_owned(Arc<store_detail::Owner> owner, F function)
    -> StorageFuture<typename decltype(function(rstd::declval<Storage&>()))::value_type> {
    using Output = decltype(function(rstd::declval<Storage&>()));
    auto pair    = rstd::async::oneshot::channel<Output>();
    auto job     = store_detail::make_job(rstd::move(function), rstd::move(pair.template get<0>()));
    return submit(rstd::move(owner), rstd::move(job), rstd::move(pair.template get<1>()));
}

template<class F>
auto Store::call(F function) const {
    return call_owned(owner_.clone(), rstd::move(function));
}

auto Store::submit_object(store_detail::Job                               job,
                          rstd::async::oneshot::Receiver<DbResult<empty>> reply) const
    -> StorageFuture<empty> {
    return submit(owner_.clone(), rstd::move(job), rstd::move(reply));
}

auto Store::write_object(Storage& storage, ObjectMetadata meta, slice<u8> bytes,
                         mut_ref<dyn<FnMut<bool()>>> valid) -> DbResult<empty> {
    if (! valid->operator()()) return Err(Error::invalidated());
    auto key    = meta.key.clone();
    auto result = storage.put_object(rstd::move(meta), bytes);
    if (! valid->operator()()) {
        rstd_try(
            storage.db.execute(store_detail::delete_object, parameter("key"_str, key.as_str())));
        return Err(Error::invalidated());
    }
    return result;
}

auto Store::shutdown_owned(Arc<store_detail::Owner> owner) -> StorageFuture<empty> {
    auto shutdown      = co_await owner->shutdown.lock();
    auto stopped       = rstd_co_try(rstd::async::spawn_blocking([owner = owner.clone()] {
                                   return owner->stop_worker();
                                     }),
                                     Error::io);
    auto worker_result = rstd_co_try(co_await rstd::move(stopped),
                                     Error::state("cache file worker join failed"_str));
    rstd_co_try(worker_result);
    if (! owner->control->closed.load(Ordering::Acquire)) {
        rstd_co_try(co_await call_owned(owner.clone(), [](Storage& storage) -> DbResult<empty> {
            storage.cleanup_garbage(rstd::time::Instant::now(), true);
            rstd_try(storage.db.checkpoint());
            storage.stopped = true;
            return Ok(empty {});
        }));
    }
    auto finishing = rstd_co_try(rstd::async::spawn_blocking([owner = owner.clone()] {
                                     return owner->join_storage();
                                 }),
                                 Error::io);
    co_return rstd_co_try(co_await rstd::move(finishing),
                          Error::state("cache storage join failed"_str));
}

auto Store::clone() const -> Store { return Store(owner_.clone()); }

auto Store::open(PathBuf root) -> StorageFuture<Store> {
    auto notification = rstd_co_try(rstd::async::Notify::make(), Error::io);
    auto control      = Arc<store_detail::Control>::make(rstd::move(notification));
    auto channel      = rstd::sync::mpmc::sync_channel<store_detail::Job>(usize(8));
    auto owner = Arc<store_detail::Owner>::make(rstd::move(channel.get<0>()), control.clone());
    auto ready = rstd::async::oneshot::channel<DbResult<empty>>();
    auto actor = rstd_co_try(rstd::thread::spawn([root     = rstd::move(root),
                                                  receiver = rstd::move(channel.get<1>()),
                                                  control  = control.clone(),
                                                  ready    = rstd::move(ready.get<0>())]() mutable {
                                 return store_detail::storage_worker(rstd::move(root),
                                                                     rstd::move(receiver),
                                                                     rstd::move(control),
                                                                     rstd::move(ready));
                             }),
                             Error::io);
    {
        auto guard = owner->storage.lock().unwrap();
        *guard     = Some(rstd::move(actor));
    }
    auto initialized = rstd_co_try(co_await rstd::move(ready.get<1>()),
                                   Error::state("cache storage stopped before ready"_str));
    rstd_co_try(initialized);
    auto handle       = rstd_co_try(owner->sender_handle());
    auto materializer = rstd_co_try(
        rstd::thread::spawn([sender = rstd::move(handle), control = rstd::move(control)]() mutable {
            return store_detail::worker(rstd::move(sender), rstd::move(control));
        }),
        Error::io);
    {
        auto guard = owner->materializer.lock().unwrap();
        *guard     = Some(rstd::move(materializer));
    }
    co_return Ok(Store(rstd::move(owner)));
}

auto Store::lookup(String key) const -> StorageFuture<Option<Version>> {
    auto control = owner_->control.clone();
    return call([key = rstd::move(key), control = rstd::move(control)](Storage& s) mutable {
        auto result = s.lookup(key.as_str());
        control->notify();
        return result;
    });
}

auto Store::create(Metadata meta) const -> StorageFuture<Version> {
    auto control = owner_->control.clone();
    return call([meta = rstd::move(meta), control = rstd::move(control)](Storage& s) mutable {
        auto result = s.create(rstd::move(meta));
        control->notify();
        return result;
    });
}

auto Store::read(Version version, u64 block) const -> StorageFuture<Option<Bytes>> {
    return call([version = rstd::move(version), block](Storage& s) {
        return s.read(version.id, block);
    });
}

auto Store::write(Version version, Blocks blocks) const -> StorageFuture<CommitOutcome> {
    return call([version = rstd::move(version),
                 blocks  = rstd::move(blocks),
                 control = owner_->control.clone()](Storage& s) {
        auto result = s.write(version.id, blocks);
        if (result.is_ok() && result->newly_complete) control->notify();
        return result;
    });
}

auto Store::complete(Version version) const -> StorageFuture<bool> {
    return call([version = rstd::move(version)](Storage& s) -> DbResult<bool> {
        rstd_try(s.repair_segments(version.id));
        auto value = rstd_try(s.load(version.id));
        return Ok(value->index.is_committed() && value->index.complete());
    });
}

auto Store::covered(Version version, Range range) const -> StorageFuture<bool> {
    return call([version = rstd::move(version), range](Storage& s) -> DbResult<bool> {
        rstd_try(s.repair_segments(version.id));
        auto value = rstd_try(s.load(version.id));
        auto block = value->meta.geometry.block;
        auto end   = range.end / block + (range.end % block != u64() ? u64(1) : u64());
        for (auto i = range.start / block; i < end; ++i)
            if (value->index.get(i).is_none()) return Ok(false);
        return Ok(true);
    });
}

auto Store::update(Version version, Metadata meta) const -> StorageFuture<empty> {
    return call([version = rstd::move(version), meta = rstd::move(meta)](Storage& s) mutable {
        return s.update(version.id, rstd::move(meta));
    });
}

auto Store::missing_range(Version version, Range range) const -> StorageFuture<Range> {
    return call([version = rstd::move(version), range](Storage& s) -> DbResult<Range> {
        rstd_try(s.repair_segments(version.id));
        auto value = rstd_try(s.load(version.id));
        auto block = value->meta.geometry.block;
        auto end   = range.end;
        auto last  = range.end / block + (range.end % block != u64() ? u64(1) : u64());
        for (auto i = range.start / block + u64(1); i < last; ++i)
            if (value->index.get(i).is_some()) {
                end = i * block;
                break;
            }
        return Ok(Range { range.start, end });
    });
}

auto Store::invalidate(i64 provider) const -> StorageFuture<empty> {
    return call([provider](Storage& s) {
        return s.invalidate(provider);
    });
}

auto Store::usage() const -> StorageFuture<Usage> {
    return call([](Storage& s) {
        return s.usage();
    });
}

auto Store::checkpoint() const -> StorageFuture<empty> {
    return call([](Storage& s) -> DbResult<empty> {
        rstd_try(s.db.checkpoint());
        return Ok(empty {});
    });
}

auto Store::retire(Version version) const -> StorageFuture<empty> {
    return call([version = rstd::move(version)](Storage& s) -> DbResult<empty> {
        rstd_try(s.db.execute(store_detail::retire_version, parameter("id"_str, version.id)));
        return Ok(empty {});
    });
}

auto Store::shutdown() const -> StorageFuture<empty> { return shutdown_owned(owner_.clone()); }

auto Store::object(String key) const -> StorageFuture<Option<Object>> {
    return call([key = rstd::move(key)](Storage& s) {
        return s.object(key.as_str());
    });
}

auto Store::remove_object(String key) const -> StorageFuture<empty> {
    return call([key = rstd::move(key)](Storage& s) -> DbResult<empty> {
        rstd_try(s.db.execute(store_detail::delete_object, parameter("key"_str, key.as_str())));
        return Ok(empty {});
    });
}

auto Store::update_object(ObjectMetadata meta) const -> StorageFuture<empty> {
    return call([meta = rstd::move(meta)](Storage& s) -> DbResult<empty> {
        auto encoded = rstd_try(encode_object_metadata(meta));
        rstd_try(s.db.execute(store_detail::update_object,
                              parameter("meta"_str, encoded.as_str()),
                              parameter("key"_str, meta.key.as_str())));
        return Ok(empty {});
    });
}

auto Store::window(Version version, Range range) const -> StorageFuture<Window> {
    return call([version = rstd::move(version), range](Storage& s) -> DbResult<Window> {
        rstd_try(s.check_active(version.id));
        auto meta = rstd_try(s.load(version.id))->meta.clone();
        if (range.start > range.end || range.end > meta.geometry.length ||
            range.end - range.start > WINDOW_BYTES)
            return Err(Error::state("invalid cache window"_str));
        auto size = meta.layout.segment_bytes();
        if (size.is_none()) return Ok(Window {});
        return Ok(s.pin_segments(
            version.id,
            Range { range.start / *size,
                    range.end / *size + (range.end % *size != u64() ? u64(1) : u64()) }));
    });
}

auto Store::local_file(Version version, String client) const -> StorageFuture<Option<LocalFile>> {
    return call([version = rstd::move(version), client = rstd::move(client)](Storage& s) {
        return s.acquire_file(version.id, client.as_str());
    });
}

auto Store::renew_file(String token, String client) const -> StorageFuture<bool> {
    return call([token = rstd::move(token), client = rstd::move(client)](Storage& s) {
        return s.renew_file(token.as_str(), client.as_str());
    });
}

auto Store::release_file(String token, String client) const -> StorageFuture<empty> {
    return call(
        [token = rstd::move(token), client = rstd::move(client)](Storage& s) -> DbResult<empty> {
            rstd_try(s.db.execute(store_detail::release_lease,
                                  parameter("token"_str, token.as_str()),
                                  parameter("client"_str, client.as_str())));
            return s.evict(u64());
        });
}
} // namespace qcm::backend::resource_cache
