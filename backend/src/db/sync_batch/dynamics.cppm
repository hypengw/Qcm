module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:db.sync_batch.dynamics;
export import :db.sync_batch.item_reference;
export import :model.dynamic;
import liorm.sqlito;
import :db.codec;
import :db.schema;
import :db.timestamp;
import :db.sync.run;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace dynamic_query = liorm::query;

export namespace qcm::backend
{
[[pmacro::invoke("qcm::entity_input", DynamicInput, DynamicFields, Dynamic, {
    is_external: bool,
    remote_play_count: i64,
    remote_last_played_at: Option<Timestamp>,
    favorite_at: Option<Timestamp>,
},
                 {
                     favorite: Patch<bool>,
                 })]];
} // namespace qcm::backend

namespace qcm::backend
{
auto dynamic_contains_id(slice<i64> ids, i64 id) -> bool {
    for (const auto& value : ids)
        if (value == id) return true;
    return false;
}
inline constexpr auto dynamic_ensure = dynamic_query::compile([](dynamic_query::QueryBuilder& b) {
    auto table = b.use(database_tables).table<Dynamic>();
    return b.insert_into(table.source(), table.col<&Dynamic::id>())
        .values(b.param<i64>("id"_str))
        .on_conflict_do_nothing(table.col<&Dynamic::id>());
});
auto find_dynamic(sqlito::Transaction& tx, i64 id) -> DbWriteResult<Option<Dynamic>> {
    return Ok(rstd_try((liorm::sqlito::find<database_tables, Dynamic>(tx, id))));
}
} // namespace qcm::backend

export namespace qcm::backend
{
auto write_legacy_dynamics(sqlito::Transaction& tx, i64 provider, slice<Dynamic> entries,
                           slice<String> include, slice<String> exclude) -> DbWriteResult<empty> {
    auto known = [](ref<str> name) {
        return name == "id"_str || name == "is_external"_str || name == "play_count"_str ||
               name == "remote_play_count"_str || name == "last_position"_str ||
               name == "last_played_at"_str || name == "remote_last_played_at"_str ||
               name == "favorite_at"_str || name == "update_at"_str;
    };
    bool has_include = false;
    for (const auto& field : include)
        if (known(field.as_str())) has_include = true;
    auto allows = [&](ref<str> name) {
        bool included = ! has_include;
        for (const auto& field : include)
            if (field.as_str() == name) included = true;
        for (const auto& field : exclude)
            if (field.as_str() == name) return false;
        return included;
    };
    for (const auto& entry : entries) {
        rstd_try(resolve(tx, provider, ItemRef::Id(entry.id), slice<ItemType> {}));
        auto    old = rstd_try(find_dynamic(tx, entry.id));
        Dynamic model { entry.id,
                        entry.is_external,
                        entry.play_count,
                        entry.remote_play_count,
                        entry.last_position,
                        entry.last_played_at,
                        entry.remote_last_played_at,
                        entry.favorite_at,
                        entry.update_at };
        if (old.is_none()) {
            rstd_try(liorm::sqlito::insert_all<database_tables>(tx, rstd::move(model)));
            continue;
        }
        liorm::Changeset<Dynamic> changes;
        if (allows("is_external"_str)) changes.set<&Dynamic::is_external>(entry.is_external);
        if (allows("play_count"_str)) changes.set<&Dynamic::play_count>(entry.play_count);
        if (allows("remote_play_count"_str))
            changes.set<&Dynamic::remote_play_count>(entry.remote_play_count);
        if (allows("last_position"_str)) changes.set<&Dynamic::last_position>(entry.last_position);
        if (allows("last_played_at"_str))
            changes.set<&Dynamic::last_played_at>(entry.last_played_at);
        if (allows("remote_last_played_at"_str))
            changes.set<&Dynamic::remote_last_played_at>(entry.remote_last_played_at);
        if (allows("favorite_at"_str)) changes.set<&Dynamic::favorite_at>(entry.favorite_at);
        if (allows("update_at"_str)) changes.set<&Dynamic::update_at>(entry.update_at);
        if (changes.empty()) continue;
        rstd_try(liorm::sqlito::update<database_tables>(tx, entry.id, rstd::move(changes)));
    }
    return Ok(empty {});
}
// The caller owns the transaction, the mask prevalidation and the favorite guard.
auto write_dynamics(sqlito::Transaction& tx, i64 provider, slice<DynamicInput> entries,
                    const Mask& mask, const SyncGuard& guard) -> DbWriteResult<Vec<Dynamic>> {
    using C = liorm::Columns<Dynamic>;
    Vec<Dynamic> result;
    Vec<i64>     seen;
    for (const auto& entry : entries) {
        auto item = rstd_try(resolve(tx, provider, entry.item, slice<ItemType> {}));
        if (dynamic_contains_id(seen.as_slice(), item.id))
            return Err(DbWriteError::Invalid(String::make("duplicate dynamic identity"_str)));
        seen.push(i64(item.id));
        auto old             = rstd_try(find_dynamic(tx, item.id));
        auto inserted        = old.is_none();
        auto model           = inserted ? Dynamic { item.id, false,  i64(),  i64(),      None(),
                                                    None(),  None(), None(), Timestamp() }
                                        : rstd::move(old).unwrap();
        auto favorite_before = model.favorite_at;
        rstd_try(mask.validate<DynamicInput>());
        const auto& validated = mask;
        if (entry.favorite.is_set()) {
            if (entry.favorite_at.is_set())
                return Err(
                    DbWriteError::Invalid(String::make("favorite and favorite_at conflict"_str)));
            if (inserted || validated.allows("favorite"_str)) {
                if (entry.favorite.value()) {
                    if (model.favorite_at.is_none()) model.favorite_at = Some(Timestamp::now());
                } else {
                    model.favorite_at = None();
                }
            }
        }
        rstd_try(entry.apply(model, inserted, validated));
        if (guard.favorite_changed_after(item.id)) model.favorite_at = rstd::move(favorite_before);
        if (model.remote_play_count < i64())
            return Err(DbWriteError::Invalid(String::make("negative remote play count"_str)));
        model.update_at = Timestamp::now();
        if (inserted) {
            auto written =
                rstd_try(liorm::sqlito::insert_all<database_tables>(tx, rstd::move(model)));
            result.push(rstd::move(written));
        } else {
            liorm::Changeset<Dynamic> changes { C::is_external       = model.is_external,
                                                C::remote_play_count = model.remote_play_count,
                                                C::remote_last_played_at =
                                                    model.remote_last_played_at,
                                                C::favorite_at = model.favorite_at,
                                                C::update_at   = model.update_at };
            auto                      written =
                rstd_try(liorm::sqlito::update<database_tables>(tx, model.id, rstd::move(changes)));
            result.push(rstd::move(written));
        }
    }
    return Ok(rstd::move(result));
}
inline constexpr auto dynamic_record_play =
    dynamic_query::compile([](dynamic_query::QueryBuilder& b) {
        auto row = b.use(database_tables).table<Dynamic>();
        return b
            .insert_into(row.source(),
                         row.col<&Dynamic::id>(),
                         row.col<&Dynamic::play_count>(),
                         row.col<&Dynamic::last_played_at>())
            .values(b.param<i64>("id"_str), b.integer(1), b.param<Timestamp>("timestamp"_str))
            .on_conflict_do_update(
                array<dynamic_query::Expr, 1> { row.col<&Dynamic::id>() }.as_slice(),
                array<dynamic_query::Expr, 2> { row.col<&Dynamic::play_count>().set(
                                                    row.col<&Dynamic::play_count>() + b.integer(1)),
                                                row.col<&Dynamic::last_played_at>().set(b.excluded(
                                                    row.col<&Dynamic::last_played_at>())) }
                    .as_slice());
    });
auto record_play(sqlito::Transaction& tx, i64 song_id, Option<i64> album_id, Timestamp timestamp)
    -> DbWriteResult<empty> {
    auto play = [&](i64 item_id) -> DbWriteResult<empty> {
        auto stmt = rstd_try(liorm::sqlito::prepare<dynamic_record_play>(
            tx,
            liorm::parameters(liorm::param<"id">(item_id), liorm::param<"timestamp">(timestamp))));
        rstd_try(liorm::sqlito::discard(stmt));
        return Ok(empty {});
    };
    rstd_try(play(song_id));
    if (album_id.is_some()) rstd_try(play(*album_id));
    return Ok(empty {});
}
// Ensures a dynamic row exists for every item allocated in this batch.
auto ensure_dynamic_rows(sqlito::Transaction& tx, slice<i64> item_ids) -> DbWriteResult<empty> {
    for (const auto& id : item_ids) {
        auto stmt = rstd_try(
            liorm::sqlito::prepare<dynamic_ensure>(tx, liorm::parameters(liorm::param<"id">(id))));
        rstd_try(liorm::sqlito::discard(stmt));
    }
    return Ok(empty {});
}
} // namespace qcm::backend
