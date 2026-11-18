module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.sync.run;
import :uuid;
import :db.schema;
import :model.item;
import :model.mix_model;
import :model.song;
import :model.rel_song_artist;
import :model.rel_album_artist;
import liorm.sqlito;
using rstd::tuple;
using rstd::collections::HashMap;
using rstd::sync::Arc;
using rstd::sync::Mutex;
using rstd::sync::MutexGuard;
using rstd::sync::RwLock;
using rstd::sync::RwLockReadGuard;
using rstd::sync::RwLockWriteGuard;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::collections::BTreeSet;
using rstd::collections::HashSet;
namespace write_query     = liorm::query;
namespace retention_query = liorm::query;

namespace qcm::backend
{

auto sync_guard_error(ref<str> message) -> sqlito::Error {
    return sqlito::Error::from(sqlito::ErrorKind::Input, 0, String::make(message));
}

auto try_new_instance_id() -> Option<String> { return random_uuid().ok(); }
auto new_instance_id() -> String {
    return random_uuid().expect("cannot generate instance UUID"_str);
}

ProjectionWriteLease::ProjectionWriteLease(ProjectionState owner)
    : owner_(rstd::move(owner)), lock_(owner_->read().unwrap()) {}

ProjectionRead::ProjectionRead(ProjectionState owner, RwLockWriteGuard<empty> lock,
                               ProjectionSnapshot snapshot)
    : owner(rstd::move(owner)), lock(rstd::move(lock)), snapshot(rstd::move(snapshot)) {}

ItemRetention::ItemRetention(ProviderStateHandle state, Vec<i64> roots, Vec<i64> ids)
    : state_(rstd::move(state)), roots_(rstd::move(roots)), ids_(rstd::move(ids)) {}

ItemRetention::~ItemRetention() {
    auto state = state_->lock().unwrap();
    for (auto id : ids_.as_slice()) {
        auto found = state->retained.get_mut(id);
        if (found.is_some() && --**found == usize()) (void)state->retained.remove(id);
    }
}

auto ItemRetention::roots() const -> slice<i64> { return roots_.as_slice(); }

ProviderWriteGuard::ProviderWriteGuard(ProviderStateHandle            state,
                                       Option<ProjectionWriteLock>    projection,
                                       MutexGuard<ProviderWriteState> lock, u64 token, u64 revision,
                                       bool finalize)
    : state_(rstd::move(state)),
      projection_(rstd::move(projection)),
      lock_(rstd::move(lock)),
      token_(token),
      revision_(revision),
      finalize_(finalize) {}

auto ProviderWriteGuard::check_origin() const -> Result<empty, sqlito::Error> {
    if ((token_ != u64() && lock_->active != token_) || (finalize_ && lock_->revision != revision_))
        return Err(sync_guard_error("sync session is stale or conflicted"_str));
    return Ok(empty {});
}

auto ProviderWriteGuard::favorite_changed_after(i64 item) const -> bool {
    if (token_ == u64()) return false;
    auto found = lock_->favorites.get(item);
    return lock_->favorite_overflow > revision_ || (found.is_some() && **found > revision_);
}

auto ProviderWriteGuard::favorite(i64 item) -> void {
    if (lock_->active == u64()) return;
    if (lock_->favorites.len() < usize(200000))
        (void)lock_->favorites.insert(item, lock_->revision);
    else
        lock_->favorite_overflow = lock_->revision;
}

auto ProviderWriteGuard::invalidate_sync() -> void { lock_->active = u64(); }

auto ProviderWriteGuard::finish_sync() -> void {
    if (token_ != u64() && lock_->active == token_) lock_->active = u64();
}

auto ProviderWriteGuard::retain(Vec<i64> roots, Vec<i64> ids) -> ItemRetentionHandle {
    for (auto id : ids.as_slice()) {
        auto found = lock_->retained.get_mut(id);
        if (found.is_some())
            ++**found;
        else
            (void)lock_->retained.insert(id, usize(1));
    }
    return ItemRetentionHandle::make(state_.clone(), rstd::move(roots), rstd::move(ids));
}

auto ProviderWriteGuard::retained_items() const -> Vec<i64> {
    Vec<i64> result;
    for (auto entry : lock_->retained.iter()) result.push(i64(*rstd::get<0>(entry)));
    return result;
}

ProviderSyncSession::ProviderSyncSession(ProviderStateHandle state, ProjectionState projection,
                                         u64 token, u64 revision)
    : state_(rstd::move(state)),
      projection_(rstd::move(projection)),
      token_(token),
      revision_(revision) {}

ProviderSyncSession::~ProviderSyncSession() { cancel(); }

auto ProviderSyncSession::write(bool finalize) -> Result<Box<ProviderWriteGuard>, sqlito::Error> {
    auto projection = ProjectionWriteLock::make(projection_.clone());
    auto lock       = state_->lock().unwrap();
    if (lock->active != token_ || (finalize && lock->revision != revision_))
        return Err(sync_guard_error("sync session is stale or conflicted"_str));
    return Ok(Box<ProviderWriteGuard>::make(state_.clone(),
                                            Some(rstd::move(projection)),
                                            rstd::move(lock),
                                            token_,
                                            revision_,
                                            finalize));
}

auto ProviderSyncSession::cancel() -> void {
    auto state = state_->lock().unwrap();
    if (state->active == token_) state->active = u64();
}

auto SyncCoordinator::provider_state(i64 provider) -> ProviderStateHandle {
    auto state = state_->lock().unwrap();
    auto found = state->providers.get(provider);
    if (found.is_some()) return (*found)->clone();
    auto value = ProviderStateHandle::make();
    (void)state->providers.insert(provider, value.clone());
    return value;
}

auto SyncCoordinator::bump_epoch(CoordinatorMetadata& metadata, i64 owner) -> void {
    for (auto& entry : metadata.epochs) {
        if (rstd::get<0>(entry) == owner) {
            rstd::get<1>(entry) = rstd::get<1>(entry) + u64(1);
            return;
        }
    }
    metadata.epochs.push({ owner, u64(1) });
}

auto SyncCoordinator::clone() const -> SyncCoordinator {
    return SyncCoordinator(state_.clone(), metadata_.clone(), projection_.clone());
}

auto SyncCoordinator::start(i64 provider) -> Result<Box<ProviderSyncSession>, sqlito::Error> {
    auto state = provider_state(provider);
    auto lock  = state->lock().unwrap();
    if (lock->active != u64()) return Err(sync_guard_error("full sync is already running"_str));
    if (lock->sequence == u64::MAX) return Err(sync_guard_error("sync generation exhausted"_str));
    lock->active = ++lock->sequence;
    lock->favorites.clear();
    lock->favorite_overflow = u64();
    return Ok(Box<ProviderSyncSession>::make(
        state.clone(), projection_.clone(), lock->active, lock->revision));
}

auto SyncCoordinator::lock_writes() -> ProjectionWriteLock {
    return ProjectionWriteLock::make(projection_.clone());
}

auto SyncCoordinator::write(i64 provider, ProjectionWriteLock projection)
    -> Result<Box<ProviderWriteGuard>, sqlito::Error> {
    auto state = provider_state(provider);
    auto lock  = state->lock().unwrap();
    if (lock->revision == u64::MAX) return Err(sync_guard_error("write revision exhausted"_str));
    auto revision = ++lock->revision;
    return Ok(Box<ProviderWriteGuard>::make(
        state.clone(), Some(rstd::move(projection)), rstd::move(lock), u64(), revision, false));
}

auto SyncCoordinator::write(i64 provider) -> Result<Box<ProviderWriteGuard>, sqlito::Error> {
    return write(provider, lock_writes());
}

auto SyncCoordinator::retain_guard(i64 provider) -> Box<ProviderWriteGuard> {
    auto state    = provider_state(provider);
    auto lock     = state->lock().unwrap();
    auto revision = lock->revision;
    return Box<ProviderWriteGuard>::make(
        state.clone(), None(), rstd::move(lock), u64(), revision, false);
}

auto SyncCoordinator::read_snapshot() const -> ProjectionSnapshot {
    auto                 metadata = metadata_->lock().unwrap();
    Vec<tuple<i64, u64>> epochs;
    for (const auto& entry : metadata->epochs.as_slice())
        epochs.push({ rstd::get<0>(entry), rstd::get<1>(entry) });
    return { metadata->instance.clone(), metadata->revision, rstd::move(epochs) };
}

auto SyncCoordinator::lock_snapshot() const -> ProjectionRead {
    auto lock = projection_->write().unwrap();
    return { projection_.clone(), rstd::move(lock), read_snapshot() };
}

auto SyncCoordinator::bump() -> void {
    auto metadata      = metadata_->lock().unwrap();
    metadata->revision = metadata->revision + u64(1);
}

auto SyncCoordinator::epoch(i64 owner) const -> u64 {
    auto metadata = metadata_->lock().unwrap();
    for (const auto& entry : metadata->epochs.as_slice())
        if (rstd::get<0>(entry) == owner) return rstd::get<1>(entry);
    return u64();
}

auto SyncCoordinator::write_revision(i64 provider) -> u64 {
    auto state = provider_state(provider);
    auto lock  = state->lock().unwrap();
    return lock->revision;
}

auto SyncCoordinator::invalidate(i64 owner) -> void {
    auto metadata = metadata_->lock().unwrap();
    bump_epoch(*metadata, owner);
    if (owner != i64()) bump_epoch(*metadata, i64());
    metadata->revision = metadata->revision + u64(1);
}

SyncCoordinator::SyncCoordinator(Arc<Mutex<CoordinatorState>>    state,
                                 Arc<Mutex<CoordinatorMetadata>> metadata,
                                 ProjectionState                 projection)
    : state_(rstd::move(state)),
      metadata_(rstd::move(metadata)),
      projection_(rstd::move(projection)) {}

SyncCoordinator::SyncCoordinator()
    : state_(Arc<Mutex<CoordinatorState>>::make()),
      metadata_(Arc<Mutex<CoordinatorMetadata>>::make(CoordinatorMetadata { new_instance_id() })),
      projection_(ProjectionState::make()) {}

namespace

{

inline constexpr auto write_item_owner = write_query::compile([](write_query::QueryBuilder& b) {
    auto item = b.use(database_tables).table<Item>();
    return b.select(item.col<&Item::provider_id>())
        .from(item.source())
        .where(item.col<&Item::id>() == b.param<i64>("id"_str));
});
inline constexpr auto write_mix_remote = write_query::compile([](write_query::QueryBuilder& b) {
    auto mix = b.use(database_tables).table<Mix>();
    return b.select(mix.col<&Mix::remote_id>())
        .from(mix.source())
        .where(mix.col<&Mix::id>() == b.param<i64>("id"_str));
});
auto                  write_bind(Result<empty, liorm::BindError<sqlito::Error>> result)
    -> Result<empty, sqlito::Error> {
    if (result.is_ok()) return Ok(empty {});
    auto error = rstd::move(result).unwrap_err();
    if (error.writer_error.is_some()) return Err(rstd::move(error.writer_error).unwrap());
    return Err(sync_guard_error("write coordination bind failed"_str));
}

inline constexpr auto retention_owner =
    retention_query::compile([](retention_query::QueryBuilder& b) {
        auto item = b.use(database_tables).table<Item>();
        return b.select(item.col<&Item::provider_id>())
            .from(item.source())
            .where(item.col<&Item::id>() == b.param<i64>("id"_str));
    });
inline constexpr auto retention_album =
    retention_query::compile([](retention_query::QueryBuilder& b) {
        auto song = b.use(database_tables).table<Song>();
        return b.select(song.col<&Song::album_id>())
            .from(song.source())
            .where(song.col<&Song::id>() == b.param<i64>("id"_str));
    });
inline constexpr auto retention_song_artists =
    retention_query::compile([](retention_query::QueryBuilder& b) {
        auto relation = b.use(database_tables).table<RelSongArtist>();
        return b.select(relation.col<&RelSongArtist::artist_id>())
            .from(relation.source())
            .where(relation.col<&RelSongArtist::song_id>() == b.param<i64>("id"_str));
    });
inline constexpr auto retention_album_artists =
    retention_query::compile([](retention_query::QueryBuilder& b) {
        auto relation = b.use(database_tables).table<RelAlbumArtist>();
        return b.select(relation.col<&RelAlbumArtist::artist_id>())
            .from(relation.source())
            .where(relation.col<&RelAlbumArtist::album_id>() == b.param<i64>("id"_str));
    });
auto retention_bind(Result<empty, liorm::BindError<sqlito::Error>> result)
    -> Result<empty, sqlito::Error> {
    if (result.is_ok()) return Ok(empty {});
    auto error = rstd::move(result).unwrap_err();
    if (error.writer_error.is_some()) return Err(rstd::move(error.writer_error).unwrap());
    return Err(sync_guard_error("retention bind failed"_str));
}

} // namespace

auto write_items_locked(SyncCoordinator& coordinator, sqlito::Connection& db, slice<i64> ids,
                        ProjectionWriteLock projection)
    -> Result<ProviderWriteGuards, sqlito::Error> {
    BTreeSet<i64> providers;
    providers.insert(i64());
    auto query = rstd_try(liorm::sqlito::prepare(db, write_item_owner));
    for (auto id : ids) {
        rstd_try(write_bind(query.bind("id"_str, id)));
        auto provider = rstd_try(liorm::sqlito::optional_scalar<i64>(
            query, usize(), liorm::sqlito::RowConsumption::First));
        if (provider.is_some()) providers.insert(*provider);
        rstd_try(query.reset());
    }
    ProviderWriteGuards guards;
    for (auto provider : providers.iter()) {
        auto guard = rstd_try(coordinator.write(i64(*provider), projection.clone()));
        guards.push(rstd::move(guard));
    }
    return Ok(rstd::move(guards));
}

auto write_items(SyncCoordinator& coordinator, sqlito::Connection& db, slice<i64> ids)
    -> Result<ProviderWriteGuards, sqlito::Error> {
    auto projection = coordinator.lock_writes();
    return write_items_locked(coordinator, db, ids, rstd::move(projection));
}

auto write_mixes(SyncCoordinator& coordinator, sqlito::Connection& db, slice<i64> mix_ids,
                 slice<i64> item_ids) -> Result<ProviderWriteGuards, sqlito::Error> {
    auto     projection = coordinator.lock_writes();
    Vec<i64> affected;
    for (auto id : item_ids) affected.push(i64(id));
    auto query = rstd_try(liorm::sqlito::prepare(db, write_mix_remote));
    for (auto id : mix_ids) {
        rstd_try(write_bind(query.bind("id"_str, id)));
        auto remote = rstd_try(liorm::sqlito::optional_scalar<Option<i64>>(
            query, usize(), liorm::sqlito::RowConsumption::First));
        if (remote.is_some() && remote->is_some()) affected.push(i64(**remote));
        rstd_try(query.reset());
    }
    return write_items_locked(coordinator, db, affected.as_slice(), rstd::move(projection));
}

auto retain_items(SyncCoordinator& coordinator, sqlito::Connection& db, i64 provider,
                  slice<i64> requested) -> Result<ItemRetentionHandle, sqlito::Error> {
    HashSet<i64> unique;
    Vec<i64>     roots;
    Vec<i64>     ids;
    for (auto id : requested) {
        if (id <= i64()) return Err(sync_guard_error("invalid retained item"_str));
        if (unique.contains(id)) continue;
        (void)unique.insert(id);
        roots.push(i64(id));
        ids.push(i64(id));
        if (roots.len() > usize(4096)) return Err(sync_guard_error("retention limit exceeded"_str));
    }
    auto guard  = coordinator.retain_guard(provider);
    auto append = [&](i64 id) {
        if (! unique.contains(id)) {
            (void)unique.insert(id);
            ids.push(i64(id));
        }
    };
    auto albums = rstd_try(liorm::sqlito::prepare(db, retention_album));
    for (auto id : roots.as_slice()) {
        rstd_try(retention_bind(albums.bind("id"_str, id)));
        auto album = rstd_try(liorm::sqlito::optional_scalar<Option<i64>>(
            albums, usize(), liorm::sqlito::RowConsumption::First));
        if (album.is_some() && album->is_some()) append(**album);
        rstd_try(albums.reset());
    }
    auto song_artists  = rstd_try(liorm::sqlito::prepare(db, retention_song_artists));
    auto album_artists = rstd_try(liorm::sqlito::prepare(db, retention_album_artists));
    auto owners        = ids.len();
    for (usize index {}; index < owners; ++index) {
        auto id = ids[index];
        rstd_try(retention_bind(song_artists.bind("id"_str, id)));
        auto song_rows =
            rstd_try(liorm::sqlito::all_rows(song_artists, liorm::TupleProjection<i64> {}));
        for (auto& row : song_rows) append(rstd::get<0>(row));
        rstd_try(song_artists.reset());
        rstd_try(retention_bind(album_artists.bind("id"_str, id)));
        auto album_rows =
            rstd_try(liorm::sqlito::all_rows(album_artists, liorm::TupleProjection<i64> {}));
        for (auto& row : album_rows) append(rstd::get<0>(row));
        rstd_try(album_artists.reset());
    }
    auto owner = rstd_try(liorm::sqlito::prepare(db, retention_owner));
    for (auto id : ids.as_slice()) {
        rstd_try(retention_bind(owner.bind("id"_str, id)));
        auto current = rstd_try(liorm::sqlito::optional_scalar<i64>(
            owner, usize(), liorm::sqlito::RowConsumption::First));
        if (current.is_none() || *current != provider)
            return Err(
                sync_guard_error("retained item is missing or belongs to another provider"_str));
        rstd_try(owner.reset());
    }
    return Ok(guard->retain(rstd::move(roots), rstd::move(ids)));
}

} // namespace qcm::backend
