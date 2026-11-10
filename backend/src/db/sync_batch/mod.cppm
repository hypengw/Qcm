module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:db.sync_batch;
export import :db.sync_batch.item_reference;
export import :db.sync_batch.artists;
export import :db.sync_batch.albums;
export import :db.sync_batch.songs;
export import :db.sync_batch.song_albums;
export import :db.sync_batch.relations;
export import :db.sync_batch.dynamics;
export import :db.sync_batch.images;
export import :db.sync_batch.libraries;
export import :db.sync_batch.mix_contents;
export import :model.remote_mix;
export import :model.mix_model;
import liorm.sqlito;
import :db.codec;
import :db.schema;
import :db.timestamp;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace remote_query = liorm::query;

export namespace qcm::backend
{
[[pmacro::invoke("qcm::entity_input", RemoteMixInput, RemoteMixFields, RemoteMix, {
    name: String,
    description: Option<String>,
    track_count: i32,
    mix_type: String,
})]];
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", decode_only, ignore_unknown)]] RadioQueueInput {
    String native_id;
    String name;
    [[pmacro::helper("serde::derive", default)]]
    Option<String> description;
};
} // namespace qcm::backend

namespace qcm::backend
{
inline constexpr remote_query::ProjectionLayout<remote_query::ModelGroup<RemoteMix>>
                                                                               remote_projection;
inline constexpr remote_query::ProjectionLayout<remote_query::ModelGroup<Mix>> cache_projection;
inline constexpr auto remote_find = remote_query::compile([](remote_query::QueryBuilder& b) {
    auto t = b.use(database_tables).table<RemoteMix>();
    return b.select(remote_query::project(remote_projection, t).as_slice())
        .from(t.source())
        .where(t.col<&RemoteMix::id>() == b.param<i64>("id"_str));
});
inline constexpr auto cache_find  = remote_query::compile([](remote_query::QueryBuilder& b) {
    auto t = b.use(database_tables).table<Mix>();
    return b.select(remote_query::project(cache_projection, t).as_slice())
        .from(t.source())
        .where(t.col<&Mix::remote_id>() == b.param<i64>("id"_str))
        .limit(b.integer(1));
});
auto find_remote(sqlito::Transaction& tx, i64 id) -> DbWriteResult<Option<RemoteMix>> {
    return Ok(
        rstd_try(liorm::sqlito::fetch_model<remote_find>(tx,
                                                         remote_projection.group<0>(),
                                                         liorm::parameters(liorm::param<"id">(id)),
                                                         liorm::sqlito::RowConsumption::Drain)));
}
} // namespace qcm::backend

export namespace qcm::backend
{
// The caller owns the transaction and runs the write stages in batch order.
auto write_remote_mix(sqlito::Transaction& tx, i64 provider, const RemoteMixInput& input,
                      const Mask& mask) -> DbWriteResult<RemoteMix> {
    rstd_try(mask.validate<RemoteMixInput>());
    array<ItemType, 2> expected { ItemType::Mix, ItemType::Radio };
    auto               item     = rstd_try(resolve(tx, provider, input.item, expected.as_slice()));
    auto               old      = rstd_try(find_remote(tx, item.id));
    auto               inserted = old.is_none();
    auto               model =
        inserted ? RemoteMix { item.id, String::make(), None(), i32(), String::make("normal"_str) }
                 : rstd::move(old).unwrap();
    rstd_try(input.apply(model, inserted, mask));
    if (model.name.is_empty())
        return Err(DbWriteError::Invalid(String::make("entity name is required"_str)));
    if (inserted)
        return Ok(rstd_try(liorm::sqlito::insert_all<database_tables>(tx, rstd::move(model))));
    return Ok(rstd_try(liorm::sqlito::update_all<database_tables>(tx, rstd::move(model))));
}
auto sync_remote_mix_cache(sqlito::Transaction& tx, const RemoteMix& remote,
                           bool validate_track_count = true) -> DbWriteResult<empty> {
    using C = liorm::Columns<Mix>;
    if (validate_track_count && remote.track_count < i32())
        return Err(DbWriteError::Invalid(String::make("negative mix quantity"_str)));
    auto old         = rstd_try(liorm::sqlito::fetch_model<cache_find>(
        tx, cache_projection.group<0>(), liorm::parameters(liorm::param<"id">(remote.id))));
    auto description = remote.description.is_some() ? remote.description->clone() : String::make();
    if (old.is_some()) {
        liorm::Changeset<Mix> changes { C::name        = remote.name.clone(),
                                        C::track_count = remote.track_count,
                                        C::description = rstd::move(description),
                                        C::update_at   = Timestamp::now() };
        rstd_try(liorm::sqlito::update<database_tables>(tx, old->id, rstd::move(changes)));
    } else {
        liorm::InsertValues<Mix> values { C::name        = remote.name.clone(),
                                          C::track_count = remote.track_count,
                                          C::description = rstd::move(description),
                                          C::mix_type    = MixType::Cache,
                                          C::remote_id   = Some(i64(remote.id)),
                                          C::create_at   = Timestamp::now(),
                                          C::update_at   = Timestamp::now() };
        rstd_try(liorm::sqlito::insert<database_tables>(tx, rstd::move(values)));
    }
    return Ok(empty {});
}
auto write_legacy_remote_models(sqlito::Transaction& tx, i64 provider, slice<RemoteMix> entries,
                                const Mask& mask) -> DbWriteResult<Vec<i64>> {
    rstd_try(mask.validate<RemoteMixInput>());
    const auto&        selected = mask;
    array<ItemType, 2> expected { ItemType::Mix, ItemType::Radio };
    Vec<i64>           ids;
    for (const auto& entry : entries) {
        rstd_try(resolve(tx, provider, ItemRef::Id(entry.id), expected.as_slice()));
        auto old = rstd_try(find_remote(tx, entry.id));
        if (old.is_none()) {
            RemoteMix model { entry.id,
                              entry.name.clone(),
                              entry.description.clone(),
                              entry.track_count,
                              entry.mix_type.clone() };
            rstd_try(liorm::sqlito::insert_all<database_tables>(tx, rstd::move(model)));
        } else {
            liorm::Changeset<RemoteMix> changes;
            sync_schema::Definition<RemoteMixFields>::apply_changes(changes, entry, selected);
            if (! changes.empty())
                rstd_try(liorm::sqlito::update<database_tables>(tx, entry.id, rstd::move(changes)));
        }
        ids.push(i64(entry.id));
    }
    return Ok(rstd::move(ids));
}
auto write_legacy_remote_mixes(sqlito::Transaction& tx, i64 provider, slice<RemoteMix> entries,
                               const Mask& mask) -> DbWriteResult<Vec<i64>> {
    auto ids = rstd_try(write_legacy_remote_models(tx, provider, entries, mask));
    for (const auto& entry : entries) rstd_try(sync_remote_mix_cache(tx, entry, false));
    return Ok(rstd::move(ids));
}
auto write_legacy_radio_queues(sqlito::Transaction& tx, i64 provider,
                               slice<RadioQueueInput> entries, Timestamp now)
    -> DbWriteResult<Vec<i64>> {
    Vec<LegacyItemInput> items;
    for (const auto& entry : entries)
        items.push(
            LegacyItemInput { entry.native_id.clone(), None(), ItemType::Radio, now, now, now });
    auto           allocated = rstd_try(allocate_legacy_items(tx, provider, items.as_slice()));
    Vec<RemoteMix> models;
    for (usize index {}; index < entries.len(); ++index) {
        const auto& entry = entries[index];
        models.push(RemoteMix { allocated[index],
                                entry.name.clone(),
                                entry.description.clone(),
                                i32(),
                                String::make("queue"_str) });
    }
    return write_legacy_remote_models(tx, provider, models.as_slice(), Mask {});
}
auto read_remote_mix(sqlito::Transaction& tx, i64 id) -> DbWriteResult<RemoteMix> {
    auto found = rstd_try(find_remote(tx, id));
    if (found.is_none()) return Err(DbWriteError::Invalid(String::make("missing remote mix"_str)));
    return Ok(rstd::move(found).unwrap());
}
} // namespace qcm::backend
