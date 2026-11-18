module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.sync.state;
import liorm.sqlito;
import :db.schema;
import :model.item;
import :model.library;
import :db.codec;
import :model.mix_model;
import :model.rel_album_artist;
import :model.rel_song_artist;
import :db.sync.run;
using rstd::collections::BTreeMap;
using rstd::collections::HashSet;
using rstd::time::SystemTime;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace sync_query = liorm::query;

namespace qcm::backend
{
namespace
{
constexpr usize sync_prune_candidate_limit { 200000 };
template<bool Global>
inline constexpr auto sync_scope_items    = sync_query::compile([](sync_query::QueryBuilder& b) {
    auto item  = b.use(database_tables).table<Item>();
    auto where = (item.col<&Item::provider_id>() == b.param<i64>("provider"_str)) &&
                 (item.col<&Item::type>() == b.param<i64>("type"_str));
    if constexpr (Global)
        where = where && item.col<&Item::library_id>().is_null();
    else
        where = where && (item.col<&Item::library_id>() == b.param<i64>("library"_str));
    return b.select(item.col<&Item::id>()).from(item.source()).where(where);
});
inline constexpr auto sync_delete_item    = sync_query::compile([](sync_query::QueryBuilder& b) {
    auto item = b.use(database_tables).table<Item>();
    return b.delete_from(item.source()).where(item.col<&Item::id>() == b.param<i64>("id"_str));
});
inline constexpr auto sync_library_owner  = sync_query::compile([](sync_query::QueryBuilder& b) {
    auto library = b.use(database_tables).table<Library>();
    return b.select(library.col<&Library::provider_id>())
        .from(library.source())
        .where(library.col<&Library::id>() == b.param<i64>("library"_str));
});
inline constexpr auto sync_provider_items = sync_query::compile([](sync_query::QueryBuilder& b) {
    auto item = b.use(database_tables).table<Item>();
    return b.select(item.col<&Item::id>())
        .from(item.source())
        .where(item.col<&Item::provider_id>() == b.param<i64>("provider"_str));
});
inline constexpr auto sync_provider_libraries =
    sync_query::compile([](sync_query::QueryBuilder& b) {
        auto library = b.use(database_tables).table<Library>();
        return b.select(library.col<&Library::id>())
            .from(library.source())
            .where(library.col<&Library::provider_id>() == b.param<i64>("provider"_str));
    });
inline constexpr auto sync_delete_library    = sync_query::compile([](sync_query::QueryBuilder& b) {
    auto library = b.use(database_tables).table<Library>();
    return b.delete_from(library.source())
        .where(library.col<&Library::id>() == b.param<i64>("id"_str));
});
inline constexpr auto sync_delete_cached_mix = sync_query::compile([](sync_query::QueryBuilder& b) {
    auto mix = b.use(database_tables).table<Mix>();
    return b.delete_from(mix.source()).where(mix.col<&Mix::remote_id>() == b.param<i64>("id"_str));
});
inline constexpr sync_query::ProjectionLayout<sync_query::ModelGroup<Library>>
    legacy_library_projection;
inline constexpr sync_query::ProjectionLayout<sync_query::ModelGroup<Item>> legacy_item_projection;
inline constexpr auto legacy_libraries = sync_query::compile([](sync_query::QueryBuilder& b) {
    auto library = b.use(database_tables).table<Library>();
    return b.select(sync_query::project(legacy_library_projection, library).as_slice())
        .from(library.source())
        .where(library.col<&Library::provider_id>() == b.param<i64>("provider"_str));
});
inline constexpr auto legacy_items     = sync_query::compile([](sync_query::QueryBuilder& b) {
    auto item = b.use(database_tables).table<Item>();
    return b.select(item.col<&Item::id>())
        .from(item.source())
        .where((item.col<&Item::provider_id>() == b.param<i64>("provider"_str)) &&
               (item.col<&Item::last_sync_at>() < b.param<Timestamp>("cutoff"_str)));
});
template<class Relation, auto Owner>
inline constexpr auto legacy_relation_owners = sync_query::compile([](sync_query::QueryBuilder& b) {
    auto schema   = b.use(database_tables);
    auto item     = schema.table<Item>();
    auto relation = schema.table<Relation>();
    return b.select(sync_query::project(legacy_item_projection, item).as_slice())
        .from(relation.source())
        .join(item.source(), relation.template col<Owner>() == item.col<&Item::id>())
        .where((item.col<&Item::provider_id>() == b.param<i64>("provider"_str)) &&
               item.col<&Item::library_id>().is_not_null() &&
               (relation.template col<&Relation::update_at>() < b.param<Timestamp>("cutoff"_str)));
});
template<class Relation, auto Owner>
inline constexpr auto legacy_delete_relations =
    sync_query::compile([](sync_query::QueryBuilder& b) {
        auto relation = b.use(database_tables).table<Relation>();
        return b.delete_from(relation.source())
            .where(relation.template col<Owner>() == b.param<i64>("owner"_str));
    });
auto sync_bind(Result<empty, liorm::BindError<sqlito::Error>> result)
    -> Result<empty, sqlito::Error> {
    if (result.is_ok()) return Ok(empty {});
    auto error = rstd::move(result).unwrap_err();
    if (error.writer_error.is_some()) return Err(rstd::move(error.writer_error).unwrap());
    return Err(sqlito::Error::from(
        sqlito::ErrorKind::Unbound, 0, String::make("sync prune bind failed"_str)));
}
} // namespace

auto SyncRunState::find_scope(Option<i64> library_id, i64 item_type) -> Option<usize> {
    auto key   = ScopeKey { library_id.is_some(), library_id.unwrap_or(i64()), item_type };
    auto found = scope_index_.get(key);
    if (found.is_none()) return None();
    return Some(usize(**found));
}

auto SyncRunState::add_scope(ObserveScope scope) -> void {
    auto key =
        ScopeKey { scope.library_id.is_some(), scope.library_id.unwrap_or(i64()), scope.item_type };
    scope_index_.insert(rstd::move(key), scopes_.len());
    scopes_.push(rstd::move(scope));
}

auto SyncRunState::reject(ref<str> message) -> Result<empty, sqlito::Error> {
    failed_ = true;
    return Err(sqlito::Error::from(sqlito::ErrorKind::Input, 0, String::make(message)));
}

auto SyncRunState::has_library(i64 id) const -> bool { return library_index_.contains(id); }

auto SyncRunState::reset(i64 provider, bool explicit_mode) -> void {
    provider_ = provider;
    scopes_   = Vec<ObserveScope>();
    scope_index_.clear();
    seen_               = HashSet<i64>();
    libraries_          = Vec<i64>();
    library_index_      = HashSet<i64>();
    explicit_           = explicit_mode;
    declared_           = false;
    libraries_complete_ = false;
    sealed_             = false;
    failed_             = false;
    auto elapsed        = SystemTime::now().duration_since(SystemTime::unix_epoch()).unwrap();
    started_            = { i64(elapsed.as_secs().to_primitive()),
                            static_cast<unsigned>(elapsed.subsec_nanos().to_primitive()) };
    started_millis_     = Timestamp::from_millis(i64(elapsed.as_millis().to_primitive()));
}

auto SyncRunState::provider() const -> i64 { return provider_; }

auto SyncRunState::check_explicit_write() -> Result<empty, sqlito::Error> {
    if (! explicit_ || failed_ || sealed_) return reject("sync session is not writable"_str);
    return Ok(empty {});
}

auto SyncRunState::observe_libraries(sqlito::Connection& db, slice<i64> libraries)
    -> Result<empty, sqlito::Error> {
    rstd_try(check_explicit_write());
    if (declared_) return reject("libraries must be observed before scopes are declared"_str);
    if (libraries.len() > sync_prune_candidate_limit ||
        libraries_.len() > sync_prune_candidate_limit - libraries.len())
        return reject("observation limit exceeded"_str);
    if (libraries.is_empty()) return Ok(empty {});
    auto observe = [&]() -> Result<empty, sqlito::Error> {
        auto query = rstd_try(liorm::sqlito::prepare(db, sync_library_owner));
        for (auto id : libraries) {
            rstd_try(sync_bind(query.bind("library"_str, id)));
            if (rstd_try(query.step()) != sqlito::Step::Row)
                return reject("missing observed library"_str);
            auto owner = rstd_try(query.reader().read<i64>(usize()));
            if (owner != provider_)
                return reject("observed library belongs to another provider"_str);
            if (! has_library(id)) {
                libraries_.push(i64(id));
                (void)library_index_.insert(id);
            }
            rstd_try(query.reset());
        }
        return Ok(empty {});
    };
    auto result = observe();
    if (result.is_err()) fail();
    return result;
}

auto SyncRunState::declare_explicit(slice<ObserveScope> scopes, slice<ItemType> required)
    -> Result<empty, sqlito::Error> {
    rstd_try(check_explicit_write());
    if (declared_) return reject("scopes have already been declared"_str);
    if (scopes.len() > sync_prune_candidate_limit)
        return reject("duplicate or excessive scopes"_str);
    for (const auto& scope : scopes) {
        if (find_scope(scope.library_id, scope.item_type).is_some())
            return reject("duplicate sync scope"_str);
        if (scope.library_id.is_some()) {
            bool supported = false;
            for (auto kind : required)
                if (scope.item_type == i64(static_cast<int>(kind))) supported = true;
            if (! has_library(*scope.library_id) || ! supported)
                return reject("scope is outside declared libraries or types"_str);
        } else if (scope.item_type != i64(static_cast<int>(ItemType::Mix)) &&
                   scope.item_type != i64(static_cast<int>(ItemType::Radio))) {
            return reject("invalid global sync scope"_str);
        }
        ObserveScope copy;
        if (scope.library_id.is_some()) copy.library_id = Some(i64(*scope.library_id));
        copy.item_type = scope.item_type;
        add_scope(rstd::move(copy));
    }
    for (auto id : libraries_.as_slice())
        for (auto kind : required)
            if (find_scope(Some(i64(id)), i64(static_cast<int>(kind))).is_none())
                return reject("required library scope is missing"_str);
    if (find_scope(None(), i64(static_cast<int>(ItemType::Mix))).is_none())
        return reject("global mix scope is missing"_str);
    declared_ = true;
    return Ok(empty {});
}

auto SyncRunState::begin_explicit(sqlito::Connection& db, slice<i64> libraries,
                                  slice<ObserveScope> scopes, slice<ItemType> required)
    -> Result<empty, sqlito::Error> {
    rstd_try(observe_libraries(db, libraries));
    return declare_explicit(scopes, required);
}

auto SyncRunState::check_explicit_scope(Option<i64> library_id, i64 item_type)
    -> Result<empty, sqlito::Error> {
    if (! explicit_ || ! declared_ || failed_ || sealed_)
        return reject("sync session is not writable"_str);
    auto index = find_scope(library_id, item_type);
    if (index.is_none() || scopes_[*index].complete)
        return reject("sync scope is not writable"_str);
    return Ok(empty {});
}

auto SyncRunState::observe_explicit(Option<i64> library_id, i64 item_type, slice<i64> items)
    -> Result<empty, sqlito::Error> {
    rstd_try(check_explicit_scope(library_id, item_type));
    if (items.len() > sync_prune_candidate_limit ||
        seen_.len() > sync_prune_candidate_limit - items.len())
        return reject("sync observation limit exceeded"_str);
    for (auto id : items) (void)seen_.insert(id);
    return Ok(empty {});
}

auto SyncRunState::complete_explicit(Option<i64> library_id, i64 item_type)
    -> Result<empty, sqlito::Error> {
    if (! explicit_ || ! declared_ || failed_ || sealed_)
        return reject("sync session is not writable"_str);
    auto index = find_scope(library_id, item_type);
    if (index.is_none()) return reject("unknown sync scope"_str);
    scopes_[*index].complete = true;
    return Ok(empty {});
}

auto SyncRunState::complete_explicit_libraries() -> Result<empty, sqlito::Error> {
    if (! explicit_ || ! declared_ || failed_ || sealed_)
        return reject("sync session is not writable"_str);
    libraries_complete_ = true;
    return Ok(empty {});
}

auto SyncRunState::seal_explicit() -> Result<empty, sqlito::Error> {
    if (! explicit_ || ! declared_ || failed_ || sealed_ || ! libraries_complete_)
        return reject("sync enumeration is incomplete"_str);
    for (const auto& scope : scopes_.as_slice())
        if (! scope.complete) return reject("sync enumeration is incomplete"_str);
    sealed_ = true;
    return Ok(empty {});
}

auto SyncRunState::finalize(sqlito::Connection& db, const SyncGuard& guard, slice<i64> retained)
    -> Result<u32, sqlito::Error> {
    if (! explicit_ || failed_ || ! sealed_)
        return Err(sqlito::Error::from(
            sqlito::ErrorKind::Input, 0, String::make("sync was not sealed successfully"_str)));
    rstd_try(guard.check_origin());
    auto tx        = rstd_try(db.begin());
    auto all_items = rstd_try(liorm::sqlito::prepare(tx, sync_provider_items));
    rstd_try(sync_bind(all_items.bind("provider"_str, provider_)));
    usize count {};
    for (;;) {
        if (rstd_try(all_items.step()) == sqlito::Step::Done) break;
        count = count + usize(1);
        if (count > sync_prune_candidate_limit)
            return Err(
                sqlito::Error::from(sqlito::ErrorKind::Range,
                                    0,
                                    String::make("sync prune candidate limit exceeded"_str)));
    }
    HashSet<i64> protected_items;
    for (auto id : retained) (void)protected_items.insert(id);
    Vec<i64> candidates;
    for (const auto& scope : scopes_.as_slice()) {
        const auto& compiled =
            scope.library_id.is_some() ? sync_scope_items<false> : sync_scope_items<true>;
        auto query = rstd_try(liorm::sqlito::prepare(tx, compiled));
        rstd_try(sync_bind(query.bind("provider"_str, provider_)));
        rstd_try(sync_bind(query.bind("type"_str, scope.item_type)));
        if (scope.library_id.is_some())
            rstd_try(sync_bind(query.bind("library"_str, *scope.library_id)));
        for (;;) {
            if (rstd_try(query.step()) == sqlito::Step::Done) break;
            auto id = rstd_try(query.reader().read<i64>(usize()));
            if (! seen_.contains(id) && ! protected_items.contains(id)) candidates.push(i64(id));
        }
    }
    auto libraries = rstd_try(liorm::sqlito::prepare(tx, sync_provider_libraries));
    rstd_try(sync_bind(libraries.bind("provider"_str, provider_)));
    Vec<i64> removed_libraries;
    count = usize();
    for (;;) {
        if (rstd_try(libraries.step()) == sqlito::Step::Done) break;
        count = count + usize(1);
        if (count > sync_prune_candidate_limit)
            return Err(sqlito::Error::from(
                sqlito::ErrorKind::Range, 0, String::make("sync library limit exceeded"_str)));
        auto id = rstd_try(libraries.reader().read<i64>(usize()));
        if (! has_library(id)) removed_libraries.push(i64(id));
    }
    auto cached = rstd_try(liorm::sqlito::prepare(tx, sync_delete_cached_mix));
    auto item   = rstd_try(liorm::sqlito::prepare(tx, sync_delete_item));
    for (auto id : candidates.as_slice()) {
        rstd_try(sync_bind(cached.bind("id"_str, id)));
        rstd_try(liorm::sqlito::discard(cached));
        rstd_try(cached.reset());
        rstd_try(sync_bind(item.bind("id"_str, id)));
        rstd_try(liorm::sqlito::discard(item));
        rstd_try(item.reset());
    }
    auto library = rstd_try(liorm::sqlito::prepare(tx, sync_delete_library));
    for (auto id : removed_libraries.as_slice()) {
        rstd_try(sync_bind(library.bind("id"_str, id)));
        rstd_try(liorm::sqlito::discard(library));
        rstd_try(library.reset());
    }
    rstd_try(guard.check_origin());
    rstd_try(tx.commit());
    sealed_ = false;
    return Ok(u32(static_cast<unsigned int>(candidates.len().to_primitive())));
}

auto SyncRunState::observe(Option<i64> library_id, i64 item_type, i64 item_id) -> void {
    auto index = find_scope(library_id, item_type);
    if (index.is_none()) {
        ObserveScope scope;
        if (library_id.is_some())
            scope.library_id = Some(*library_id);
        else
            scope.library_id = None();
        scope.item_type = item_type;
        add_scope(rstd::move(scope));
        index = Some(scopes_.len() - usize(1));
    }
    if (scopes_[*index].complete) return;
    (void)seen_.insert(item_id);
}

auto SyncRunState::complete(Option<i64> library_id, i64 item_type) -> void {
    auto index = find_scope(library_id, item_type);
    if (index.is_some()) scopes_[*index].complete = true;
}

auto SyncRunState::complete_libraries() -> void { libraries_complete_ = true; }

auto SyncRunState::seal() -> void { sealed_ = true; }

auto SyncRunState::fail() -> void { failed_ = true; }

auto SyncRunState::finalize_legacy(sqlito::Connection& db, const SyncGuard& guard,
                                   slice<i64> retained) -> Result<u32, sqlito::Error> {
    if (explicit_ || failed_)
        return Err(sqlito::Error::from(
            sqlito::ErrorKind::Input, 0, String::make("legacy sync failed"_str)));
    rstd_try(guard.check_origin());
    auto         tx = rstd_try(db.begin());
    HashSet<i64> protected_items;
    for (auto id : retained) (void)protected_items.insert(id);

    auto libraries = rstd_try(liorm::sqlito::prepare(tx, legacy_libraries));
    rstd_try(sync_bind(libraries.bind("provider"_str, provider_)));
    Vec<i64> expired_libraries;
    for (;;) {
        if (rstd_try(libraries.step()) == sqlito::Step::Done) break;
        auto reader  = libraries.reader();
        auto library = rstd_try(liorm::decode_model(legacy_library_projection.group<0>(), reader));
        if (library.edit_time.seconds < started_.seconds ||
            (library.edit_time.seconds == started_.seconds &&
             library.edit_time.nanoseconds < started_.nanoseconds))
            expired_libraries.push(i64(library.id));
    }
    auto remove_library = rstd_try(liorm::sqlito::prepare(tx, sync_delete_library));
    for (auto id : expired_libraries.as_slice()) {
        rstd_try(sync_bind(remove_library.bind("id"_str, id)));
        rstd_try(liorm::sqlito::discard(remove_library));
        rstd_try(remove_library.reset());
    }

    auto remove_relations = [&]<class Relation, auto Owner>() -> Result<empty, sqlito::Error> {
        auto query = rstd_try(liorm::sqlito::prepare(tx, legacy_relation_owners<Relation, Owner>));
        rstd_try(sync_bind(query.bind("provider"_str, provider_)));
        rstd_try(sync_bind(query.bind("cutoff"_str, started_millis_)));
        HashSet<i64> seen_owners;
        Vec<i64>     owners;
        for (;;) {
            if (rstd_try(query.step()) == sqlito::Step::Done) break;
            auto reader = query.reader();
            auto item   = rstd_try(liorm::decode_model(legacy_item_projection.group<0>(), reader));
            if ((! protected_items.contains(item.id) || item.last_sync_at >= started_millis_) &&
                ! seen_owners.contains(item.id)) {
                (void)seen_owners.insert(item.id);
                owners.push(i64(item.id));
            }
        }
        auto deletion =
            rstd_try(liorm::sqlito::prepare(tx, legacy_delete_relations<Relation, Owner>));
        for (auto id : owners.as_slice()) {
            rstd_try(sync_bind(deletion.bind("owner"_str, id)));
            rstd_try(liorm::sqlito::discard(deletion));
            rstd_try(deletion.reset());
        }
        return Ok(empty {});
    };
    rstd_try((remove_relations.template operator()<RelAlbumArtist, &RelAlbumArtist::album_id>()));
    rstd_try((remove_relations.template operator()<RelSongArtist, &RelSongArtist::song_id>()));

    auto items = rstd_try(liorm::sqlito::prepare(tx, legacy_items));
    rstd_try(sync_bind(items.bind("provider"_str, provider_)));
    rstd_try(sync_bind(items.bind("cutoff"_str, started_millis_)));
    Vec<i64> expired_items;
    for (;;) {
        if (rstd_try(items.step()) == sqlito::Step::Done) break;
        auto id = rstd_try(items.reader().read<i64>(usize()));
        if (! protected_items.contains(id)) expired_items.push(i64(id));
    }
    auto remove_item = rstd_try(liorm::sqlito::prepare(tx, sync_delete_item));
    for (auto id : expired_items.as_slice()) {
        rstd_try(sync_bind(remove_item.bind("id"_str, id)));
        rstd_try(liorm::sqlito::discard(remove_item));
        rstd_try(remove_item.reset());
    }
    rstd_try(guard.check_origin());
    rstd_try(tx.commit());
    return Ok(u32(static_cast<unsigned int>(expired_items.len().to_primitive())));
}

auto SyncRunState::prune(sqlito::Connection& db) -> Result<u32, sqlito::Error> {
    if (explicit_)
        return Err(
            sqlito::Error::from(sqlito::ErrorKind::Input,
                                0,
                                String::make("explicit sync requires guarded finalization"_str)));
    if (failed_)
        return Err(sqlito::Error::from(
            sqlito::ErrorKind::Input, 0, String::make("sync observation failed"_str)));
    if (! sealed_) {
        if (explicit_)
            return Err(sqlito::Error::from(
                sqlito::ErrorKind::Input, 0, String::make("sync was not sealed"_str)));
        return Ok(u32());
    }
    auto     tx = rstd_try(db.begin());
    u32      removed {};
    usize    scanned {};
    Vec<i64> candidates;
    for (const auto& scope : scopes_.as_slice()) {
        if (! scope.complete) continue;
        const auto& compiled =
            scope.library_id.is_some() ? sync_scope_items<false> : sync_scope_items<true>;
        auto query = rstd_try(liorm::sqlito::prepare(tx, compiled));
        rstd_try(sync_bind(query.bind("provider"_str, provider_)));
        rstd_try(sync_bind(query.bind("type"_str, scope.item_type)));
        if (scope.library_id.is_some())
            rstd_try(sync_bind(query.bind("library"_str, *scope.library_id)));
        for (;;) {
            auto step = rstd_try(query.step());
            if (step == sqlito::Step::Done) break;
            scanned = scanned + usize(1);
            if (scanned > sync_prune_candidate_limit)
                return Err(
                    sqlito::Error::from(sqlito::ErrorKind::Range,
                                        0,
                                        String::make("sync prune candidate limit exceeded"_str)));
            auto reader = query.reader();
            auto id     = rstd_try(reader.read<i64>(usize()));
            if (! seen_.contains(id)) candidates.push(i64(id));
        }
    }
    auto deletion = rstd_try(liorm::sqlito::prepare(tx, sync_delete_item));
    for (const auto id : candidates.as_slice()) {
        rstd_try(sync_bind(deletion.bind("id"_str, id)));
        rstd_try(liorm::sqlito::discard(deletion));
        removed = removed + u32(1);
        rstd_try(deletion.reset());
    }
    rstd_try(tx.commit());
    return Ok(removed);
}
} // namespace qcm::backend
