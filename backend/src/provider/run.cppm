module;
#include <rstd/enum.hpp>
export module qcm.backend:provider.run;
export import luato;
export import :provider.dto;
export import :provider.generation;
export import rstd;
export import sqlito;
export import :db.sync_batch.batch;
export import :db.sync.state;
using alloc::rc::Rc;
using rstd::async::coro;
using rstd::sync::Arc;
using rstd::sync::Mutex;
using rstd::sync::atomic::Atomic;
using namespace rstd::prelude;

namespace qcm::backend
{
struct LuaRunWorkerState;
struct LuaRunCaller;
} // namespace qcm::backend

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

struct LuaRunCancellation {
    Atomic<bool>                           cancelled { false };
    Atomic<bool>                           failed { false };
    Mutex<Option<ProviderGenerationCheck>> generation { None() };

    auto bind_generation(ProviderGenerationCheck check) -> Result<empty, sqlito::Error>;
    auto is_valid() const -> bool;
    auto check() const -> Result<empty, sqlito::Error>;
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

    LuaRunWorker(SyncCoordinator coordinator, i64 provider, bool explicit_plugin);
};
struct LuaRunOutput {
    LuaRunResult value { LuaRunResult::Unit() };
    i64          albums {}, artists {}, songs {};
};

auto run_error(ref<str> message) -> luato::Error;
auto execute_run_command(sqlito::Connection& db, LuaRunWorker& worker,
                         const LuaRunCancellation& cancellation, LuaRunCommand command)
    -> Result<LuaRunOutput, luato::Error>;
auto sync_song_album_ids_impl(sqlito::Connection& db, i64 provider, i64 library,
                              slice<LegacySongAlbumInput> inputs, const SyncGuard& guard)
    -> Result<LuaRunResult, luato::Error>;

class ProviderLibraryRun : public LuaRunBridge {
public:
    using Predicate = Box<dyn<Fn<bool()>>>;
    using Progress  = Box<dyn<FnMut<coro<empty>(SyncProgressKind, i32)>>>;

private:
    sqlito::ExecutorHandle db_;
    Arc<LuaRunWorkerState> state_;
    Rc<LuaRunCaller>       caller_;
    i64                    provider_;
    bool                   tracks_sync_ {};

    static auto execute_owned(sqlito::ExecutorHandle db, Arc<LuaRunWorkerState> state,
                              Rc<LuaRunCaller> caller, LuaRunCommand command)
        -> coro<Result<LuaRunResult, luato::Error>>;
    static auto rejected(luato::Error error) -> coro<Result<LuaRunResult, luato::Error>>;
    auto        set_writable(Predicate writable) -> void;

protected:
    auto worker_state() const -> Arc<LuaRunWorkerState>;
    auto caller_state() const -> Rc<LuaRunCaller>;
    auto database() const -> sqlito::ExecutorHandle { return db_.clone(); }
    auto track_sync() -> void { tracks_sync_ = true; }

public:
    ProviderLibraryRun(sqlito::ExecutorHandle db, SyncCoordinator& coordinator, i64 provider,
                       bool explicit_plugin);
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
        set_writable(Predicate::make(rstd::move(writable)));
    }
    ProviderLibraryRun(const ProviderLibraryRun&) = delete;
    ~ProviderLibraryRun() override;
    auto cancel_call() -> void override;
    auto set_valid(Predicate valid) -> void;
    template<class Valid>
    auto set_valid(Valid valid) -> void {
        set_valid(Predicate::make(rstd::move(valid)));
    }
    auto set_progress(Progress progress) -> void;
    template<class Callback>
    auto set_progress(Callback progress) -> void {
        set_progress(Progress::make(rstd::move(progress)));
    }
    auto songs() const -> i64;
    auto is_valid() const -> bool override;
    auto provider_id() const -> i64 override { return provider_; }
    auto fail_sync() -> void override;
    auto bind_generation(ProviderGenerationCheck generation)
        -> Result<empty, luato::Error> override;
    auto execute(LuaRunCommand command) -> coro<Result<LuaRunResult, luato::Error>> override;
};
class ProviderSyncRun final : public ProviderLibraryRun {
    static auto start_owned(sqlito::ExecutorHandle db, Arc<LuaRunWorkerState> state,
                            Rc<LuaRunCaller> caller) -> coro<Result<empty, luato::Error>>;
    static auto finalize_owned(sqlito::ExecutorHandle db, Arc<LuaRunWorkerState> state,
                               Rc<LuaRunCaller> caller) -> coro<Result<u32, luato::Error>>;

public:
    ProviderSyncRun(sqlito::ExecutorHandle db, SyncCoordinator& coordinator, i64 provider,
                    bool explicit_plugin, bool explicit_run, Vec<ItemType> required_types,
                    bool full_sync = true);
    auto start() -> coro<Result<empty, luato::Error>>;
    auto finalize() -> coro<Result<u32, luato::Error>>;
};
// Calls and their userdata stay on the Lua VM's current thread.
class LuaCallState {
    LuaRunBridge*                   bridge_;
    Option<ProviderGenerationCheck> generation_;

    static auto rejected() -> coro<Result<LuaRunResult, luato::Error>>;
    static auto execute_owned(Rc<LuaCallState> state, coro<Result<LuaRunResult, luato::Error>> task)
        -> coro<Result<LuaRunResult, luato::Error>>;

public:
    explicit LuaCallState(LuaRunBridge* bridge): bridge_(bridge) {}
    auto        check() const -> Result<empty, luato::Error>;
    auto        invalidate() -> void;
    auto        provider_id() const -> Result<i64, luato::Error>;
    auto        bind_generation(ProviderGenerationCheck generation) -> Result<empty, luato::Error>;
    auto        fail_sync() -> void;
    static auto execute(Rc<LuaCallState> state, LuaRunCommand command)
        -> coro<Result<LuaRunResult, luato::Error>>;
};
class LuaCallGuard {
    Rc<LuaCallState> state_;

public:
    explicit LuaCallGuard(LuaRunBridge* bridge);
    LuaCallGuard(const LuaCallGuard&)                    = delete;
    auto operator=(const LuaCallGuard&) -> LuaCallGuard& = delete;
    ~LuaCallGuard();
    auto state() const -> Rc<LuaCallState> { return state_.clone(); }
};
} // namespace qcm::backend
