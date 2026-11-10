module;
#include <rstd/enum.hpp>
#include <rstd/macro.hpp>
export module qcm.backend:provider.run;
export import luato;
export import :provider.dto;
export import :provider.lua_error;
export import :provider.generation;
export import rstd;
export import sqlito;
export import :db.sync_batch;
export import :provider.batch;
import :db.database;
import :db.sync.run;
export import :db.sync.state;
import rstd.log;
import :db.schema;
import :db.timestamp;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::coro;
using rstd::sync::Arc;
using rstd::sync::Mutex;
using rstd::sync::atomic::Atomic;
using rstd::sync::atomic::Ordering;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
class LuaRunResult {
    RSTD_ENUM(
        LuaRunResult,
        (Unit),
        (Ids, (Vec<i64> values;)),
        (Libraries, (Vec<Library> values;)),
        (Items, (Vec<Item> values;)),
        (Batch, (SyncResult value;)),
        (Observed, (i64 count;))
    )
};
struct LuaRunResultDefinition {
    static constexpr bool serializable   = true;
    static constexpr bool deserializable = false;
    template<class S>
    static auto serialize(S& sink, const LuaRunResult& result) -> typename S::result_type {
        RSTD_MATCH(result) {
            RSTD_CASE(Unit) { return sink.serialize_unit(); }
            RSTD_CASE(Ids, values) { return rstd::serde::serialize(sink, values); }
            RSTD_CASE(Libraries, values) { return rstd::serde::serialize(sink, values); }
            RSTD_CASE(Items, values) { return rstd::serde::serialize(sink, values); }
            RSTD_CASE(Batch, value) { return rstd::serde::serialize(sink, value); }
            RSTD_CASE(Observed, count) { return rstd::serde::serialize(sink, count); }
        }
    }
};
constexpr auto rstd_serde_definition(LuaRunResult*) { return LuaRunResultDefinition {}; }
struct LegacySyncOptions {
    Vec<String> include;
    Vec<String> exclude;
};
constexpr auto rstd_serde_definition(LegacySyncOptions*) {
    using namespace rstd::serde;
    return RecordDefinition<
        LegacySyncOptions,
        "LuaSyncOption",
        Direction::Deserialize,
        UnknownFieldPolicy::Ignore,
        Member<&LegacySyncOptions::include, "include", FieldPolicy::Defaulted>,
        Member<&LegacySyncOptions::exclude, "exclude", FieldPolicy::Defaulted>> {};
}
inline auto legacy_write_guard(SyncCoordinator& coordinator, i64 provider, bool explicit_plugin,
                               ProviderSyncSession* session = nullptr)
    -> Result<Box<ProviderWriteGuard>, luato::Error> {
    if (explicit_plugin)
        return Err(luato::Error::make(luato::ErrorKind::Runtime,
                                      String::make("qcm.run"_str),
                                      String::make("explicit plugins must use sync_batch"_str)));
    auto guard = session != nullptr ? session->write() : coordinator.write(provider);
    if (guard.is_err())
        return Err(lua_batch::write_error(rstd::move(guard).unwrap_err(),
                                          "legacy write guard failed"_str));
    return Ok(rstd::move(guard).unwrap());
}
template<class F>
auto with_legacy_write(SyncCoordinator& coordinator, i64 provider, bool explicit_plugin,
                       F&& operation, SyncRunState* run = nullptr,
                       ProviderSyncSession* session = nullptr)
    -> Result<LuaRunResult, luato::Error> {
    auto locked = legacy_write_guard(coordinator, provider, explicit_plugin, session);
    if (locked.is_err()) {
        if (run != nullptr) run->fail();
        return Err(rstd::move(locked).unwrap_err());
    }
    auto guard  = rstd::move(locked).unwrap();
    auto result = operation(*guard);
    if (result.is_err() && run != nullptr) run->fail();
    if (result.is_ok()) coordinator.bump();
    return result;
}
class LuaRunCommand {
    RSTD_ENUM(
        LuaRunCommand,
        (Begin, (Vec<i64> libraries; Vec<ObserveScope> scopes;)),
        (Batch, (SyncBatch batch;)),
        (Libraries),
        (AllocateItems, (Vec<LegacyItemInput> items;)),
        (SyncLibraries, (Vec<LegacyLibraryInput> items;)),
        (SyncAlbums, (Vec<Album> items; LegacySyncOptions options;)),
        (SyncDynamics, (Vec<Dynamic> items; LegacySyncOptions options;)),
        (SyncSongs, (Vec<Song> items;)),
        (SyncImages, (Vec<LegacyImageInput> items;)),
        (SyncArtists, (Vec<Artist> items;)),
        (SyncRemoteMixes, (Vec<RemoteMix> items; LegacySyncOptions options;)),
        (SyncRemoteMixSongIds, (i64 mix_id; Vec<i64> items;)),
        (SyncRadioQueues, (Vec<RadioQueueInput> items;)),
        (SyncSongArtistIds, (Vec<LegacyRelationInput> items;)),
        (SyncAlbumArtistIds, (Vec<LegacyRelationInput> items;)),
        (SyncSongAlbumIds, (i64 library_id; Vec<LegacySongAlbumInput> items;)),
        (ObserveLibraries, (Vec<i64> libraries;)),
        (Observe, (ObserveScope scope; Vec<ItemRef> keys;)),
        (Complete, (ObserveScope scope;)),
        (CompleteLibraries),
        (Seal),
        (Resolve, (ItemType kind; Vec<String> ids;)),
        (Progress, (SyncProgressKind kind; i32 count;))
    )

public:
    auto requires_write() const -> bool {
        RSTD_MATCH(*this) {
            RSTD_CASE(Libraries) { return false; }
            RSTD_CASE(Resolve) { return false; }
            RSTD_CASE_DEFAULT { return true; }
        }
    }
};
class LuaRunBridge {
public:
    virtual ~LuaRunBridge()                 = default;
    virtual auto is_valid() const -> bool   = 0;
    virtual auto provider_id() const -> i64 = 0;
    virtual auto fail_sync() -> void        = 0;
    virtual auto cancel_call() -> void {}
    virtual auto bind_generation(ProviderGenerationCheck generation)
        -> Result<empty, luato::Error>                                                      = 0;
    virtual auto execute(LuaRunCommand command) -> coro<Result<LuaRunResult, luato::Error>> = 0;
};
inline auto allocate_items_impl(sqlito::Connection& db, i64 provider, slice<LegacyItemInput> inputs,
                                const SyncGuard& guard) -> Result<LuaRunResult, luato::Error> {
    auto begun = db.begin();
    if (begun.is_err())
        return Err(lua_batch::write_error(rstd::move(begun).unwrap_err(),
                                          "cannot begin item transaction"_str));
    auto tx        = rstd::move(begun).unwrap();
    auto allocated = allocate_legacy_items(tx, provider, inputs);
    if (allocated.is_err())
        return Err(lua_batch::write_error(rstd::move(allocated).unwrap_err(),
                                          "item allocation failed"_str));
    auto origin = guard.check_origin();
    if (origin.is_err())
        return Err(lua_batch::write_error(rstd::move(origin).unwrap_err(),
                                          "cannot commit item allocation"_str));
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(lua_batch::write_error(rstd::move(committed).unwrap_err(),
                                          "cannot commit item allocation"_str));
    return Ok(LuaRunResult::Ids(rstd::move(allocated).unwrap()));
}

inline auto sync_libraries_impl(sqlito::Connection& db, i64 provider,
                                slice<LegacyLibraryInput> inputs, const SyncGuard& guard)
    -> Result<LuaRunResult, luato::Error> {
    auto begun = db.begin();
    if (begun.is_err())
        return Err(lua_batch::write_error(rstd::move(begun).unwrap_err(),
                                          "library transaction failed"_str));
    auto tx      = rstd::move(begun).unwrap();
    auto written = write_legacy_libraries(tx, provider, inputs);
    if (written.is_err()) {
        (void)tx.rollback();
        return Err(
            lua_batch::write_error(rstd::move(written).unwrap_err(), "library write failed"_str));
    }
    auto origin = guard.check_origin();
    if (origin.is_err())
        return Err(
            lua_batch::write_error(rstd::move(origin).unwrap_err(), "library commit failed"_str));
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(lua_batch::write_error(rstd::move(committed).unwrap_err(),
                                          "library commit failed"_str));
    return Ok(LuaRunResult::Ids(rstd::move(written).unwrap()));
}

template<class Input>
auto legacy_sync_mask(const LegacySyncOptions& options, ref<str> table_name)
    -> Result<Mask, luato::Error> {
    auto table = database_tables.find_table(table_name);
    if (table.is_none())
        return Err(luato::Error::make(luato::ErrorKind::Runtime,
                                      String::make("qcm.run"_str),
                                      String::make("missing legacy sync schema"_str)));
    auto known = [&](ref<str> name) {
        auto column = database_tables.find_column(*table, name);
        return column.is_some() && database_tables.column(*column).name == name;
    };
    auto supported = [&](ref<str> name) {
        Mask        candidate;
        Vec<String> fields;
        fields.push(String::make(name));
        candidate.include = Some(rstd::move(fields));
        return candidate.template validate<Input>().is_ok();
    };
    auto excluded = [&](ref<str> name) {
        for (const auto& value : options.exclude.as_slice())
            if (value.as_str() == name) return true;
        return false;
    };
    bool        recognized_include = false;
    Vec<String> selected;
    for (const auto& value : options.include.as_slice()) {
        auto name = value.as_str();
        if (! known(name)) continue;
        recognized_include = true;
        if (! supported(name)) {
            if (excluded(name) || name == "id"_str) continue;
            return Err(luato::Error::make(luato::ErrorKind::Runtime,
                                          String::make("qcm.run"_str),
                                          String::make("unsupported legacy include field"_str)));
        }
        if (! excluded(name)) selected.push(value.clone());
    }
    Mask mask;
    if (recognized_include) {
        mask.include = Some(rstd::move(selected));
    } else {
        for (const auto& value : options.exclude.as_slice()) {
            auto name = value.as_str();
            if (! known(name)) continue;
            if (supported(name)) selected.push(value.clone());
        }
        if (! selected.is_empty()) mask.exclude = Some(rstd::move(selected));
    }
    return Ok(rstd::move(mask));
}

inline auto sync_albums_impl(sqlito::Connection& db, i64 provider, slice<Album> inputs,
                             const LegacySyncOptions& options, const SyncGuard& guard)
    -> Result<LuaRunResult, luato::Error> {
    auto mask = legacy_sync_mask<AlbumInput>(options, "album"_str);
    if (mask.is_err()) return Err(rstd::move(mask).unwrap_err());
    auto begun = db.begin();
    if (begun.is_err())
        return Err(
            lua_batch::write_error(rstd::move(begun).unwrap_err(), "album transaction failed"_str));
    auto tx      = rstd::move(begun).unwrap();
    auto written = write_legacy_albums(tx, provider, inputs, *mask);
    if (written.is_err()) {
        rstd::log::error("album write failed: {} entries={}",
                         written.as_ref().unwrap_err(),
                         inputs.len().to_primitive());
        (void)tx.rollback();
        return Err(
            lua_batch::write_error(rstd::move(written).unwrap_err(), "album write failed"_str));
    }
    auto origin = guard.check_origin();
    if (origin.is_err())
        return Err(
            lua_batch::write_error(rstd::move(origin).unwrap_err(), "album commit failed"_str));
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(
            lua_batch::write_error(rstd::move(committed).unwrap_err(), "album commit failed"_str));
    return Ok(LuaRunResult::Ids(rstd::move(written).unwrap()));
}

inline auto sync_dynamics_impl(sqlito::Connection& db, i64 provider, slice<Dynamic> inputs,
                               const LegacySyncOptions& options, const SyncGuard& guard)
    -> Result<LuaRunResult, luato::Error> {
    auto begun = db.begin();
    if (begun.is_err())
        return Err(lua_batch::write_error(rstd::move(begun).unwrap_err(),
                                          "dynamic transaction failed"_str));
    auto tx      = rstd::move(begun).unwrap();
    auto written = write_legacy_dynamics(
        tx, provider, inputs, options.include.as_slice(), options.exclude.as_slice());
    if (written.is_err()) {
        rstd::log::error("dynamic write failed: {} entries={}",
                         written.as_ref().unwrap_err(),
                         inputs.len().to_primitive());
        (void)tx.rollback();
        return Err(
            lua_batch::write_error(rstd::move(written).unwrap_err(), "dynamic write failed"_str));
    }
    auto origin = guard.check_origin();
    if (origin.is_err())
        return Err(
            lua_batch::write_error(rstd::move(origin).unwrap_err(), "dynamic commit failed"_str));
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(lua_batch::write_error(rstd::move(committed).unwrap_err(),
                                          "dynamic commit failed"_str));
    return Ok(LuaRunResult::Unit());
}

inline auto sync_songs_impl(sqlito::Connection& db, i64 provider, slice<Song> inputs,
                            const SyncGuard& guard) -> Result<LuaRunResult, luato::Error> {
    auto begun = db.begin();
    if (begun.is_err())
        return Err(
            lua_batch::write_error(rstd::move(begun).unwrap_err(), "song transaction failed"_str));
    auto tx      = rstd::move(begun).unwrap();
    auto written = write_legacy_songs(tx, provider, inputs);
    if (written.is_err()) {
        (void)tx.rollback();
        return Err(
            lua_batch::write_error(rstd::move(written).unwrap_err(), "song write failed"_str));
    }
    auto origin = guard.check_origin();
    if (origin.is_err())
        return Err(
            lua_batch::write_error(rstd::move(origin).unwrap_err(), "song commit failed"_str));
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(
            lua_batch::write_error(rstd::move(committed).unwrap_err(), "song commit failed"_str));
    return Ok(LuaRunResult::Ids(rstd::move(written).unwrap()));
}

inline auto sync_images_impl(sqlito::Connection& db, i64 provider, slice<LegacyImageInput> inputs,
                             const SyncGuard& guard) -> Result<LuaRunResult, luato::Error> {
    auto begun = db.begin();
    if (begun.is_err())
        return Err(
            lua_batch::write_error(rstd::move(begun).unwrap_err(), "image transaction failed"_str));
    auto tx      = rstd::move(begun).unwrap();
    auto written = write_legacy_images(tx, provider, inputs);
    if (written.is_err()) {
        (void)tx.rollback();
        return Err(
            lua_batch::write_error(rstd::move(written).unwrap_err(), "image write failed"_str));
    }
    auto origin = guard.check_origin();
    if (origin.is_err())
        return Err(
            lua_batch::write_error(rstd::move(origin).unwrap_err(), "image commit failed"_str));
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(
            lua_batch::write_error(rstd::move(committed).unwrap_err(), "image commit failed"_str));
    return Ok(LuaRunResult::Unit());
}

inline auto sync_artists_impl(sqlito::Connection& db, i64 provider, slice<Artist> inputs,
                              const SyncGuard& guard) -> Result<LuaRunResult, luato::Error> {
    auto begun = db.begin();
    if (begun.is_err())
        return Err(lua_batch::write_error(rstd::move(begun).unwrap_err(),
                                          "artist transaction failed"_str));
    auto tx      = rstd::move(begun).unwrap();
    auto written = write_legacy_artists(tx, provider, inputs);
    if (written.is_err()) {
        (void)tx.rollback();
        return Err(
            lua_batch::write_error(rstd::move(written).unwrap_err(), "artist write failed"_str));
    }
    auto origin = guard.check_origin();
    if (origin.is_err())
        return Err(
            lua_batch::write_error(rstd::move(origin).unwrap_err(), "artist commit failed"_str));
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(
            lua_batch::write_error(rstd::move(committed).unwrap_err(), "artist commit failed"_str));
    return Ok(LuaRunResult::Ids(rstd::move(written).unwrap()));
}

inline auto sync_remote_mixes_impl(sqlito::Connection& db, i64 provider, slice<RemoteMix> inputs,
                                   const LegacySyncOptions& options, const SyncGuard& guard)
    -> Result<LuaRunResult, luato::Error> {
    auto mask = legacy_sync_mask<RemoteMixInput>(options, "remote_mix"_str);
    if (mask.is_err()) return Err(rstd::move(mask).unwrap_err());
    auto begun = db.begin();
    if (begun.is_err())
        return Err(lua_batch::write_error(rstd::move(begun).unwrap_err(),
                                          "remote mix transaction failed"_str));
    auto tx      = rstd::move(begun).unwrap();
    auto written = write_legacy_remote_mixes(tx, provider, inputs, *mask);
    if (written.is_err()) {
        (void)tx.rollback();
        return Err(lua_batch::write_error(rstd::move(written).unwrap_err(),
                                          "remote mix write failed"_str));
    }
    auto origin = guard.check_origin();
    if (origin.is_err())
        return Err(lua_batch::write_error(rstd::move(origin).unwrap_err(),
                                          "remote mix commit failed"_str));
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(lua_batch::write_error(rstd::move(committed).unwrap_err(),
                                          "remote mix commit failed"_str));
    return Ok(LuaRunResult::Ids(rstd::move(written).unwrap()));
}

inline auto write_relations_impl(sqlito::Connection& db, i64 provider,
                                 slice<LegacyRelationInput> inputs, bool album_relation,
                                 const SyncGuard& guard) -> Result<LuaRunResult, luato::Error> {
    auto begun = db.begin();
    if (begun.is_err())
        return Err(lua_batch::write_error(rstd::move(begun).unwrap_err(),
                                          "relation transaction failed"_str));
    auto tx     = rstd::move(begun).unwrap();
    auto result = album_relation ? write_legacy_album_artists(tx, provider, inputs)
                                 : write_legacy_song_artists(tx, provider, inputs);
    if (result.is_err()) {
        (void)tx.rollback();
        return Err(
            lua_batch::write_error(rstd::move(result).unwrap_err(), "relation write failed"_str));
    }
    auto origin = guard.check_origin();
    if (origin.is_err())
        return Err(
            lua_batch::write_error(rstd::move(origin).unwrap_err(), "relation commit failed"_str));
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(lua_batch::write_error(rstd::move(committed).unwrap_err(),
                                          "relation commit failed"_str));
    return Ok(LuaRunResult::Unit());
}

inline auto sync_album_artist_ids_impl(sqlito::Connection& db, i64 provider,
                                       slice<LegacyRelationInput> items, const SyncGuard& guard)
    -> Result<LuaRunResult, luato::Error> {
    return write_relations_impl(db, provider, items, true, guard);
}
inline auto sync_song_artist_ids_impl(sqlito::Connection& db, i64 provider,
                                      slice<LegacyRelationInput> items, const SyncGuard& guard)
    -> Result<LuaRunResult, luato::Error> {
    return write_relations_impl(db, provider, items, false, guard);
}
inline auto sync_song_album_ids_impl(sqlito::Connection& db, i64 provider, i64 library,
                                     slice<LegacySongAlbumInput> items, const SyncGuard& guard)
    -> Result<LuaRunResult, luato::Error> {
    auto storage = [](auto error) {
        return lua_batch::write_error(rstd::move(error), "song album sync failed"_str);
    };
    auto tx = rstd_try(db.begin(), storage);
    rstd_try(write_legacy_song_album_ids(tx, provider, library, items), storage);
    rstd_try(guard.check_origin(), storage);
    rstd_try(tx.commit(), storage);
    return Ok(LuaRunResult::Unit());
}

inline auto sync_remote_mix_song_ids_impl(sqlito::Connection& db, i64 provider, i64 mix_id,
                                          slice<i64> songs, const SyncGuard& guard)
    -> Result<LuaRunResult, luato::Error> {
    auto begun = db.begin();
    if (begun.is_err())
        return Err(lua_batch::write_error(rstd::move(begun).unwrap_err(),
                                          "mix contents transaction failed"_str));
    auto tx      = rstd::move(begun).unwrap();
    auto written = write_legacy_mix_songs(tx, provider, mix_id, songs);
    if (written.is_err()) {
        (void)tx.rollback();
        return Err(lua_batch::write_error(rstd::move(written).unwrap_err(),
                                          "mix contents write failed"_str));
    }
    auto origin = guard.check_origin();
    if (origin.is_err())
        return Err(lua_batch::write_error(rstd::move(origin).unwrap_err(),
                                          "mix contents commit failed"_str));
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(lua_batch::write_error(rstd::move(committed).unwrap_err(),
                                          "mix contents commit failed"_str));
    return Ok(LuaRunResult::Unit());
}

inline auto sync_radio_queues_impl(sqlito::Connection& db, i64 provider,
                                   slice<RadioQueueInput> inputs, const SyncGuard& guard)
    -> Result<LuaRunResult, luato::Error> {
    auto begun = db.begin();
    if (begun.is_err())
        return Err(lua_batch::write_error(rstd::move(begun).unwrap_err(),
                                          "radio queue transaction failed"_str));
    auto tx      = rstd::move(begun).unwrap();
    auto written = write_legacy_radio_queues(tx, provider, inputs, Timestamp::now());
    if (written.is_err()) {
        (void)tx.rollback();
        return Err(lua_batch::write_error(rstd::move(written).unwrap_err(),
                                          "radio queue write failed"_str));
    }
    auto origin = guard.check_origin();
    if (origin.is_err())
        return Err(
            lua_batch::write_error(rstd::move(origin).unwrap_err(), "radio commit failed"_str));
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(
            lua_batch::write_error(rstd::move(committed).unwrap_err(), "radio commit failed"_str));
    return Ok(LuaRunResult::Ids(rstd::move(written).unwrap()));
}

inline auto run_error(ref<str> message) -> luato::Error {
    return luato::Error::make(
        luato::ErrorKind::Runtime, String::make("qcm.run"_str), String::make(message));
}
template<class F>
auto run_database(sqlito::ExecutorHandle db, F function)
    -> coro<decltype(function(rstd::declval<sqlito::Connection&>()))> {
    using Output = decltype(function(rstd::declval<sqlito::Connection&>()));
    auto result  = rstd_co_try(
        co_await database_run(
            rstd::move(db),
            [function = rstd::move(function)](
                sqlito::Connection& connection) mutable -> Result<Output, sqlito::Error> {
                return Ok(function(connection));
            }),
        [](sqlito::Error error) {
            return lua_batch::write_error(rstd::move(error), "run worker failed"_str);
        });
    co_return rstd::move(result);
}
struct LuaRunCancellation {
    Atomic<bool>                           cancelled { false };
    Atomic<bool>                           failed { false };
    Mutex<Option<ProviderGenerationCheck>> generation { None() };

    auto bind_generation(ProviderGenerationCheck check) -> Result<empty, sqlito::Error> {
        if (! check.is_current()) return Err(sync_guard_error("provider generation changed"_str));
        auto bound = generation.lock().unwrap();
        if (bound->is_some()) {
            if (! (**bound).same_generation(check))
                return Err(sync_guard_error("provider call cannot change generation"_str));
        } else {
            bound->insert(rstd::move(check));
        }
        return Ok(empty {});
    }
    auto is_valid() const -> bool {
        if (cancelled.load(Ordering::Acquire) || failed.load(Ordering::Acquire)) return false;
        auto bound = generation.lock().unwrap();
        return bound->is_none() || (**bound).is_current();
    }
    auto check() const -> Result<empty, sqlito::Error> {
        if (cancelled.load(Ordering::Acquire) || failed.load(Ordering::Acquire))
            return Err(sync_guard_error("sync run was cancelled or failed"_str));
        auto bound = generation.lock().unwrap();
        if (bound->is_none()) return Err(sync_guard_error("provider generation is not bound"_str));
        if (! (**bound).is_current())
            return Err(sync_guard_error("provider generation changed"_str));
        return Ok(empty {});
    }
};
struct LuaRunPending {
    Arc<LuaRunCancellation> cancellation;
    bool                    completed {};
    ~LuaRunPending() {
        if (! completed) cancellation->failed.store(true, Ordering::Release);
    }
};
class LuaRunGuard final : public SyncGuard {
    const SyncGuard&          origin_;
    const LuaRunCancellation& cancellation_;

public:
    LuaRunGuard(const SyncGuard& origin, const LuaRunCancellation& cancellation)
        : origin_(origin), cancellation_(cancellation) {}
    auto check_origin() const -> Result<empty, sqlito::Error> override {
        rstd_try(cancellation_.check());
        return origin_.check_origin();
    }
    auto favorite_changed_after(i64 item) const -> bool override {
        return origin_.favorite_changed_after(item);
    }
};
class LuaRunBoundaryGuard final : public SyncGuard {
    const LuaRunCancellation& cancellation_;

public:
    explicit LuaRunBoundaryGuard(const LuaRunCancellation& cancellation)
        : cancellation_(cancellation) {}
    auto check_origin() const -> Result<empty, sqlito::Error> override {
        return cancellation_.check();
    }
    auto favorite_changed_after(i64) const -> bool override { return false; }
};
struct LuaRunWorker {
    SyncCoordinator                  coordinator;
    i64                              provider;
    bool                             explicit_plugin;
    bool                             explicit_run {};
    bool                             full_sync {};
    bool                             started {};
    bool                             finished {};
    Vec<ItemType>                    required_types;
    Option<Box<ProviderSyncSession>> session;
    SyncRunState                     run;

    LuaRunWorker(SyncCoordinator coordinator, i64 provider, bool explicit_plugin)
        : coordinator(rstd::move(coordinator)),
          provider(provider),
          explicit_plugin(explicit_plugin) {}
};
struct LuaRunWorkerState {
    Arc<LuaRunCancellation> cancellation { Arc<LuaRunCancellation>::make() };
    Mutex<LuaRunWorker>     worker;

    LuaRunWorkerState(SyncCoordinator coordinator, i64 provider, bool explicit_plugin)
        : worker(LuaRunWorker(rstd::move(coordinator), provider, explicit_plugin)) {}
};
struct LuaRunOutput {
    LuaRunResult value { LuaRunResult::Unit() };
    i64          albums {}, artists {}, songs {};
};
struct LuaRunCaller {
    using Predicate = Box<dyn<Fn<bool()>>>;
    using Progress  = Box<dyn<FnMut<coro<empty>(SyncProgressKind, i32)>>>;
    Option<Predicate>                valid;
    Option<Predicate>                writable;
    Option<Progress>                 progress;
    rstd::async::watch::Sender<bool> cancellation { false };
    i64                              albums {}, artists {}, songs {};

    auto is_valid() const -> bool { return valid.is_none() || (*valid)->operator()(); }
    auto is_writable() const -> bool { return writable.is_none() || (*writable)->operator()(); }
    auto add_progress(SyncProgressKind kind, i64 count) -> coro<empty> {
        auto cancelled = cancellation.subscribe();
        if (*cancelled.borrow()) co_return empty {};
        if (kind == SyncProgressKind::Album) albums = albums + count;
        if (kind == SyncProgressKind::Artist) artists = artists + count;
        if (kind == SyncProgressKind::Song) songs = songs + count;
        if (progress.is_some())
            (void)co_await rstd::async::select(
                cancelled.changed(),
                rstd::async::into_future(
                    (*progress)->operator()(kind, i32(static_cast<int>(count.to_primitive())))));
        co_return empty {};
    }
};
template<class F>
auto run_caller_database(sqlito::ExecutorHandle db, Arc<LuaRunCancellation> cancellation,
                         Rc<LuaRunCaller> caller, F function)
    -> coro<decltype(function(rstd::declval<sqlito::Connection&>()))> {
    if (! cancellation->is_valid() || ! caller->is_valid())
        co_return Err(run_error("no active sync run"_str));
    LuaRunPending pending { cancellation.clone() };
    auto          cancelled = caller->cancellation.subscribe();
    auto          selected  = co_await rstd::async::select(
        cancelled.changed(),
        rstd::async::into_future(run_database(rstd::move(db), rstd::move(function))));
    if (selected.is_left()) co_return Err(run_error("no active sync run"_str));
    if (! cancellation->is_valid() || ! caller->is_valid())
        co_return Err(run_error("no active sync run"_str));
    pending.completed = true;
    co_return rstd::move(selected).unwrap_right();
}
inline auto run_observe(sqlito::Connection& db, LuaRunWorker& worker,
                        const LuaRunCancellation& cancellation, const ObserveScope& scope,
                        slice<ItemRef> keys) -> Result<LuaRunResult, luato::Error> {
    rstd_try(cancellation.check(), [](auto error) {
        return lua_batch::write_error(rstd::move(error), "observation origin changed"_str);
    });
    if (worker.explicit_run) {
        auto checked = worker.run.check_explicit_scope(scope.library_id, scope.item_type);
        if (checked.is_err())
            return Err(lua_batch::write_error(rstd::move(checked).unwrap_err(),
                                              "observation scope rejected"_str));
    }
    Option<Box<ProviderWriteGuard>> guard;
    if (worker.session.is_some()) {
        auto locked = (*worker.session)->write();
        if (locked.is_err())
            return Err(lua_batch::write_error(rstd::move(locked).unwrap_err(),
                                              "observation guard rejected"_str));
        guard = Some(rstd::move(locked).unwrap());
    }
    auto begun = db.begin();
    if (begun.is_err())
        return Err(lua_batch::write_error(rstd::move(begun).unwrap_err(),
                                          "observation transaction failed"_str));
    auto     tx = rstd::move(begun).unwrap();
    Vec<i64> observed;
    for (const auto& key : keys) {
        auto item = qcm::backend::resolve(tx, worker.provider, key, slice<ItemType> {});
        if (item.is_err())
            return Err(lua_batch::write_error(rstd::move(item).unwrap_err(),
                                              "observed item resolution failed"_str));
        if (i64(static_cast<int>(item->type)) != scope.item_type ||
            item->library_id.is_some() != scope.library_id.is_some() ||
            (scope.library_id.is_some() && *item->library_id != *scope.library_id))
            return Err(run_error("observed item is outside scope"_str));
        observed.push(i64(item->id));
    }
    auto checked = cancellation.check();
    if (checked.is_err())
        return Err(
            lua_batch::write_error(rstd::move(checked).unwrap_err(), "observation cancelled"_str));
    if (guard.is_some()) {
        auto origin = (*guard)->check_origin();
        if (origin.is_err())
            return Err(lua_batch::write_error(rstd::move(origin).unwrap_err(),
                                              "observation origin changed"_str));
    }
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(lua_batch::write_error(rstd::move(committed).unwrap_err(),
                                          "observation commit failed"_str));
    if (worker.explicit_run) {
        auto result =
            worker.run.observe_explicit(scope.library_id, scope.item_type, observed.as_slice());
        if (result.is_err())
            return Err(lua_batch::write_error(rstd::move(result).unwrap_err(),
                                              "observation rejected"_str));
    } else
        for (auto id : observed.as_slice())
            worker.run.observe(scope.library_id, scope.item_type, id);
    return Ok(LuaRunResult::Observed(i64(observed.len().to_primitive())));
}
inline auto execute_run_command(sqlito::Connection& db, LuaRunWorker& worker,
                                const LuaRunCancellation& cancellation, LuaRunCommand command)
    -> Result<LuaRunOutput, luato::Error> {
    if (worker.finished) return Err(run_error("sync run was cancelled or failed"_str));
    auto current = cancellation.check();
    if (current.is_err()) {
        worker.run.fail();
        return Err(lua_batch::write_error(rstd::move(current).unwrap_err(),
                                          "provider context is no longer active"_str));
    }
    LuaRunOutput output;
    auto         legacy_write = [&](auto operation) -> Result<LuaRunResult, luato::Error> {
        auto locked = legacy_write_guard(
            worker.coordinator,
            worker.provider,
            worker.explicit_plugin,
            worker.session.is_some() ? worker.session->as_mut_ptr().as_raw_ptr() : nullptr);
        if (locked.is_err()) return Err(rstd::move(locked).unwrap_err());
        auto        guard = rstd::move(locked).unwrap();
        LuaRunGuard origin(*guard, cancellation);
        auto        result = operation(origin);
        if (result.is_ok()) worker.coordinator.bump();
        return result;
    };
    auto complete = [&](auto explicit_finish,
                        auto legacy_finish) -> Result<LuaRunResult, luato::Error> {
        if (! worker.started) return Err(run_error("sync completion is not available"_str));
        if (worker.explicit_run) {
            auto result = explicit_finish();
            if (result.is_err())
                return Err(lua_batch::write_error(rstd::move(result).unwrap_err(),
                                                  "sync completion failed"_str));
        } else {
            legacy_finish();
        }
        return Ok(LuaRunResult::Unit());
    };
    auto execute = [&]() -> Result<LuaRunResult, luato::Error> {
        RSTD_MATCH(command) {
            RSTD_CASE(Begin, libraries, scopes) {
                if (! worker.started || ! worker.full_sync || ! worker.explicit_run)
                    return Err(run_error("begin_sync requires an explicit full sync"_str));
                auto result = worker.run.begin_explicit(
                    db, libraries.as_slice(), scopes.as_slice(), worker.required_types.as_slice());
                if (result.is_err())
                    return Err(lua_batch::write_error(rstd::move(result).unwrap_err(),
                                                      "sync declaration failed"_str));
                return Ok(LuaRunResult::Unit());
            }
            RSTD_CASE(Libraries) {
                auto values = rstd_try(read_run_libraries(db, worker.provider));
                return Ok(LuaRunResult::Libraries(rstd::move(values)));
            }
            RSTD_CASE(ObserveLibraries, libraries) {
                if (! worker.started || ! worker.full_sync || ! worker.explicit_run)
                    return Err(run_error("observe_libraries requires an explicit full sync"_str));
                rstd_try(worker.run.observe_libraries(db, libraries.as_slice()), [](auto error) {
                    return lua_batch::write_error(rstd::move(error),
                                                  "library observation failed"_str);
                });
                return Ok(LuaRunResult::Unit());
            }
            RSTD_CASE(Resolve, kind, ids) {
                auto values =
                    rstd_try(resolve_run_items(db, worker.provider, kind, ids.as_slice()));
                return Ok(LuaRunResult::Items(rstd::move(values)));
            }
            RSTD_CASE(Observe, scope, keys) {
                if (! worker.started) return Err(run_error("observe items is not available"_str));
                return run_observe(db, worker, cancellation, scope, keys.as_slice());
            }
            RSTD_CASE(Complete, scope) {
                return complete(
                    [&] {
                        return worker.run.complete_explicit(scope.library_id, scope.item_type);
                    },
                    [&] {
                        worker.run.complete(scope.library_id, scope.item_type);
                    });
            }
            RSTD_CASE(CompleteLibraries) {
                return complete(
                    [&] {
                        return worker.run.complete_explicit_libraries();
                    },
                    [&] {
                        worker.run.complete_libraries();
                    });
            }
            RSTD_CASE(Seal) {
                return complete(
                    [&] {
                        return worker.run.seal_explicit();
                    },
                    [&] {
                        worker.run.seal();
                    });
            }
            RSTD_CASE(Batch, batch) {
                if (worker.explicit_run) {
                    auto checked = worker.run.check_explicit_write();
                    if (checked.is_err())
                        return Err(lua_batch::write_error(rstd::move(checked).unwrap_err(),
                                                          "sync write rejected"_str));
                }
                LuaRunBoundaryGuard boundary(cancellation);
                auto                result = write_run_batch(
                    db,
                    worker.coordinator,
                    worker.provider,
                    worker.session.is_some() ? worker.session->as_mut_ptr().as_raw_ptr() : nullptr,
                    batch,
                    boundary);
                if (result.is_err()) return Err(rstd::move(result).unwrap_err());
                output.albums  = i64(result->albums.len().to_primitive());
                output.artists = i64(result->artists.len().to_primitive());
                output.songs   = i64(result->songs.len().to_primitive());
                return Ok(LuaRunResult::Batch(rstd::move(result).unwrap()));
            }
            RSTD_CASE(Progress, kind, count) {
                switch (kind) {
                case SyncProgressKind::Album: output.albums = i64(count.to_primitive()); break;
                case SyncProgressKind::Artist: output.artists = i64(count.to_primitive()); break;
                case SyncProgressKind::Song: output.songs = i64(count.to_primitive()); break;
                }
                return Ok(LuaRunResult::Unit());
            }
            RSTD_CASE(AllocateItems, items) {
                return legacy_write([&](const SyncGuard& guard) {
                    return allocate_items_impl(db, worker.provider, items.as_slice(), guard);
                });
            }
            RSTD_CASE(SyncLibraries, items) {
                return legacy_write([&](const SyncGuard& guard) {
                    return sync_libraries_impl(db, worker.provider, items.as_slice(), guard);
                });
            }
            RSTD_CASE(SyncAlbums, items, options) {
                return legacy_write([&](const SyncGuard& guard) {
                    return sync_albums_impl(db, worker.provider, items.as_slice(), options, guard);
                });
            }
            RSTD_CASE(SyncDynamics, items, options) {
                return legacy_write([&](const SyncGuard& guard) {
                    return sync_dynamics_impl(
                        db, worker.provider, items.as_slice(), options, guard);
                });
            }
            RSTD_CASE(SyncSongs, items) {
                return legacy_write([&](const SyncGuard& guard) {
                    return sync_songs_impl(db, worker.provider, items.as_slice(), guard);
                });
            }
            RSTD_CASE(SyncImages, items) {
                return legacy_write([&](const SyncGuard& guard) {
                    return sync_images_impl(db, worker.provider, items.as_slice(), guard);
                });
            }
            RSTD_CASE(SyncArtists, items) {
                return legacy_write([&](const SyncGuard& guard) {
                    return sync_artists_impl(db, worker.provider, items.as_slice(), guard);
                });
            }
            RSTD_CASE(SyncRemoteMixes, items, options) {
                return legacy_write([&](const SyncGuard& guard) {
                    return sync_remote_mixes_impl(
                        db, worker.provider, items.as_slice(), options, guard);
                });
            }
            RSTD_CASE(SyncRemoteMixSongIds, mix_id, items) {
                return legacy_write([&](const SyncGuard& guard) {
                    return sync_remote_mix_song_ids_impl(
                        db, worker.provider, mix_id, items.as_slice(), guard);
                });
            }
            RSTD_CASE(SyncRadioQueues, items) {
                return legacy_write([&](const SyncGuard& guard) {
                    return sync_radio_queues_impl(db, worker.provider, items.as_slice(), guard);
                });
            }
            RSTD_CASE(SyncSongArtistIds, items) {
                return legacy_write([&](const SyncGuard& guard) {
                    return sync_song_artist_ids_impl(db, worker.provider, items.as_slice(), guard);
                });
            }
            RSTD_CASE(SyncSongAlbumIds, library_id, items) {
                return legacy_write([&](const SyncGuard& guard) {
                    return sync_song_album_ids_impl(
                        db, worker.provider, library_id, items.as_slice(), guard);
                });
            }
            RSTD_CASE(SyncAlbumArtistIds, items) {
                return legacy_write([&](const SyncGuard& guard) {
                    return sync_album_artist_ids_impl(db, worker.provider, items.as_slice(), guard);
                });
            }
        }
        return Err(run_error("invalid run command"_str));
    };
    auto result = execute();
    if (result.is_err()) {
        worker.run.fail();
        return Err(rstd::move(result).unwrap_err());
    }
    output.value = rstd::move(result).unwrap();
    return Ok(rstd::move(output));
}
class ProviderLibraryRun : public LuaRunBridge {
    sqlito::ExecutorHandle db_;
    Arc<LuaRunWorkerState> state_;
    Rc<LuaRunCaller>       caller_ { make_rc<LuaRunCaller>() };
    i64                    provider_;
    bool                   tracks_sync_ {};

    static auto execute_owned(sqlito::ExecutorHandle db, Arc<LuaRunWorkerState> state,
                              Rc<LuaRunCaller> caller, LuaRunCommand command)
        -> coro<Result<LuaRunResult, luato::Error>> {
        if (! state->cancellation->is_valid() || ! caller->is_valid())
            co_return Err(run_error("no active sync run"_str));
        if (command.requires_write() && ! caller->is_writable())
            co_return Err(run_error("provider session is read-only"_str));
        LuaRunPending pending { state->cancellation.clone() };
        auto          result = co_await run_caller_database(
            rstd::move(db),
            state->cancellation.clone(),
            caller.clone(),
            [state = state.clone(), command = rstd::move(command)](sqlito::Connection& db) mutable {
                auto worker = state->worker.lock().unwrap();
                return execute_run_command(db, *worker, *state->cancellation, rstd::move(command));
            });
        if (result.is_err()) {
            pending.completed = true;
            co_return Err(rstd::move(result).unwrap_err());
        }
        if (! state->cancellation->is_valid() || ! caller->is_valid())
            co_return Err(run_error("no active sync run"_str));
        if (result->albums != i64())
            co_await caller.get()->add_progress(SyncProgressKind::Album, result->albums);
        if (result->artists != i64())
            co_await caller.get()->add_progress(SyncProgressKind::Artist, result->artists);
        if (result->songs != i64())
            co_await caller.get()->add_progress(SyncProgressKind::Song, result->songs);
        if (! state->cancellation->is_valid() || ! caller->is_valid())
            co_return Err(run_error("no active sync run"_str));
        pending.completed = true;
        co_return Ok(rstd::move(result->value));
    }
    static auto rejected(luato::Error error) -> coro<Result<LuaRunResult, luato::Error>> {
        co_return Err(rstd::move(error));
    }

protected:
    auto worker_state() const -> Arc<LuaRunWorkerState> { return state_.clone(); }
    auto database() const -> sqlito::ExecutorHandle { return db_.clone(); }
    auto caller_state() const -> Rc<LuaRunCaller> { return caller_.clone(); }
    auto track_sync() -> void { tracks_sync_ = true; }

public:
    ProviderLibraryRun(sqlito::ExecutorHandle db, SyncCoordinator& coordinator, i64 provider,
                       bool explicit_plugin)
        : db_(rstd::move(db)),
          state_(Arc<LuaRunWorkerState>::make(coordinator.clone(), provider, explicit_plugin)),
          provider_(provider) {}
    template<class Valid>
    ProviderLibraryRun(sqlito::ExecutorHandle db, SyncCoordinator& coordinator, i64 provider,
                       bool explicit_plugin, Valid valid)
        : ProviderLibraryRun(rstd::move(db), coordinator, provider, explicit_plugin) {
        set_valid(rstd::move(valid));
    }
    template<class Valid, class Writable>
    ProviderLibraryRun(sqlito::ExecutorHandle db, SyncCoordinator& coordinator, i64 provider,
                       bool explicit_plugin, Valid valid, Writable writable)
        : ProviderLibraryRun(rstd::move(db), coordinator, provider, explicit_plugin,
                             rstd::move(valid)) {
        caller_.get()->writable = Some(LuaRunCaller::Predicate::make(rstd::move(writable)));
    }
    ProviderLibraryRun(const ProviderLibraryRun&) = delete;
    ~ProviderLibraryRun() override { cancel_call(); }
    auto cancel_call() -> void override {
        state_->cancellation->cancelled.store(true, Ordering::Release);
        (void)caller_->cancellation.send_replace(true);
    }
    template<class Valid>
    auto set_valid(Valid valid) -> void {
        caller_.get()->valid = Some(LuaRunCaller::Predicate::make(rstd::move(valid)));
    }
    template<class Progress>
    auto set_progress(Progress progress) -> void {
        caller_.get()->progress = Some(LuaRunCaller::Progress::make(rstd::move(progress)));
    }
    auto songs() const -> i64 { return caller_->songs; }
    auto is_valid() const -> bool override {
        return state_->cancellation->is_valid() && caller_->is_valid();
    }
    auto provider_id() const -> i64 override { return provider_; }
    auto fail_sync() -> void override {
        if (tracks_sync_) state_->cancellation->failed.store(true, Ordering::Release);
    }
    auto bind_generation(ProviderGenerationCheck generation)
        -> Result<empty, luato::Error> override {
        auto bound = state_->cancellation->bind_generation(rstd::move(generation));
        if (bound.is_err())
            return Err(lua_batch::write_error(rstd::move(bound).unwrap_err(),
                                              "provider context is no longer active"_str));
        return Ok(empty {});
    }
    auto execute(LuaRunCommand command) -> coro<Result<LuaRunResult, luato::Error>> override {
        if (! is_valid()) return rejected(run_error("no active sync run"_str));
        if (command.requires_write() && ! caller_->is_writable())
            return rejected(run_error("provider session is read-only"_str));
        return execute_owned(db_.clone(), state_.clone(), caller_.clone(), rstd::move(command));
    }
};
class ProviderSyncRun final : public ProviderLibraryRun {
    static auto start_owned(sqlito::ExecutorHandle db, Arc<LuaRunWorkerState> state,
                            Rc<LuaRunCaller> caller) -> coro<Result<empty, luato::Error>> {
        auto cancellation = state->cancellation.clone();
        co_return co_await run_caller_database(
            rstd::move(db),
            rstd::move(cancellation),
            rstd::move(caller),
            [state = rstd::move(state)](sqlito::Connection&) -> Result<empty, luato::Error> {
                auto worker = state->worker.lock().unwrap();
                if (state->cancellation->cancelled.load(Ordering::Acquire) || worker->started)
                    return Err(run_error("sync run is cancelled or already started"_str));
                rstd_try(state->cancellation->check(), [](auto error) {
                    return lua_batch::write_error(rstd::move(error),
                                                  "sync start origin changed"_str);
                });
                if (worker->full_sync) {
                    auto session = worker->coordinator.start(worker->provider);
                    if (session.is_err())
                        return Err(lua_batch::write_error(rstd::move(session).unwrap_err(),
                                                          "cannot start sync run"_str));
                    worker->session = Some(rstd::move(session).unwrap());
                }
                worker->run.reset(worker->provider, worker->explicit_run);
                worker->started = true;
                return Ok(empty {});
            });
    }
    static auto finalize_owned(sqlito::ExecutorHandle db, Arc<LuaRunWorkerState> state,
                               Rc<LuaRunCaller> caller) -> coro<Result<u32, luato::Error>> {
        auto cancellation = state->cancellation.clone();
        co_return co_await run_caller_database(
            rstd::move(db),
            rstd::move(cancellation),
            rstd::move(caller),
            [state = rstd::move(state)](sqlito::Connection& db) -> Result<u32, luato::Error> {
                auto worker = state->worker.lock().unwrap();
                if (! worker->started || worker->finished ||
                    state->cancellation->cancelled.load(Ordering::Acquire) ||
                    state->cancellation->failed.load(Ordering::Acquire))
                    return Err(run_error("sync run cannot be finalized"_str));
                rstd_try(state->cancellation->check(), [](auto error) {
                    return lua_batch::write_error(rstd::move(error),
                                                  "sync finalization origin changed"_str);
                });
                if (worker->session.is_none()) {
                    worker->finished = true;
                    return Ok(u32());
                }
                auto locked = (*worker->session)->write(true);
                if (locked.is_err())
                    return Err(lua_batch::write_error(rstd::move(locked).unwrap_err(),
                                                      "sync finalization guard rejected"_str));
                auto        guard = rstd::move(locked).unwrap();
                LuaRunGuard origin(*guard, *state->cancellation);
                auto        retained = guard->retained_items();
                auto result = worker->explicit_run
                                  ? worker->run.finalize(db, origin, retained.as_slice())
                                  : worker->run.finalize_legacy(db, origin, retained.as_slice());
                if (result.is_err())
                    return Err(lua_batch::write_error(rstd::move(result).unwrap_err(),
                                                      "sync finalization failed"_str));
                worker->coordinator.invalidate(worker->provider);
                guard->finish_sync();
                worker->finished = true;
                return Ok(rstd::move(*result));
            });
    }

public:
    ProviderSyncRun(sqlito::ExecutorHandle db, SyncCoordinator& coordinator, i64 provider,
                    bool explicit_plugin, bool explicit_run, Vec<ItemType> required_types,
                    bool full_sync = true)
        : ProviderLibraryRun(rstd::move(db), coordinator, provider, explicit_plugin) {
        track_sync();
        auto state             = worker_state();
        auto worker            = state->worker.lock().unwrap();
        worker->explicit_run   = explicit_run;
        worker->full_sync      = full_sync;
        worker->required_types = rstd::move(required_types);
    }
    auto start() -> coro<Result<empty, luato::Error>> {
        return start_owned(database(), worker_state(), caller_state());
    }
    auto finalize() -> coro<Result<u32, luato::Error>> {
        return finalize_owned(database(), worker_state(), caller_state());
    }
};
// Calls and their userdata stay on the Lua VM's current thread.
class LuaCallState {
    LuaRunBridge*                   bridge_;
    Option<ProviderGenerationCheck> generation_;

    static auto rejected() -> coro<Result<LuaRunResult, luato::Error>> {
        co_return Err(run_error("provider context is no longer active"_str));
    }
    static auto execute_owned(Rc<LuaCallState> state, coro<Result<LuaRunResult, luato::Error>> task)
        -> coro<Result<LuaRunResult, luato::Error>> {
        rstd_co_try(state->check());
        auto result = co_await rstd::move(task);
        rstd_co_try(state->check());
        co_return result;
    }

public:
    explicit LuaCallState(LuaRunBridge* bridge): bridge_(bridge) {}
    auto check() const -> Result<empty, luato::Error> {
        if (bridge_ == nullptr || ! bridge_->is_valid() ||
            (generation_.is_some() && ! generation_->is_current()))
            return Err(run_error("provider context is no longer active"_str));
        return Ok(empty {});
    }
    auto invalidate() -> void {
        auto bridge = bridge_;
        bridge_     = nullptr;
        if (bridge != nullptr) bridge->cancel_call();
    }
    auto provider_id() const -> Result<i64, luato::Error> {
        rstd_try(check());
        return Ok(bridge_->provider_id());
    }
    auto bind_generation(ProviderGenerationCheck generation) -> Result<empty, luato::Error> {
        rstd_try(check());
        if (generation_.is_some() && ! generation_->same_generation(generation))
            return Err(run_error("provider context generation changed"_str));
        rstd_try(bridge_->bind_generation(generation.clone()));
        generation_ = Some(rstd::move(generation));
        return Ok(empty {});
    }
    auto fail_sync() -> void {
        if (bridge_ != nullptr) bridge_->fail_sync();
    }
    static auto execute(Rc<LuaCallState> state, LuaRunCommand command)
        -> coro<Result<LuaRunResult, luato::Error>> {
        if (state->check().is_err()) return rejected();
        auto task = state->bridge_->execute(rstd::move(command));
        return execute_owned(rstd::move(state), rstd::move(task));
    }
};
class LuaCallGuard {
    Rc<LuaCallState> state_;

public:
    explicit LuaCallGuard(LuaRunBridge* bridge): state_(make_rc<LuaCallState>(bridge)) {}
    LuaCallGuard(const LuaCallGuard&)                    = delete;
    auto operator=(const LuaCallGuard&) -> LuaCallGuard& = delete;
    ~LuaCallGuard() { state_.get()->invalidate(); }
    auto state() const -> Rc<LuaCallState> { return state_.clone(); }
};
} // namespace qcm::backend
