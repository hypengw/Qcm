module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:db.sync_batch.relations;
export import :db.sync_batch.item_reference;
export import :db.sync_batch.mix_contents;
export import :model.rel_album_artist;
export import :model.rel_song_artist;
import liorm.sqlito;
import :db.codec;
import :db.schema;
import :db.timestamp;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace relation_query = liorm::query;

export namespace qcm::backend
{
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", decode_only)]]
RelationInput {
    ItemRef      owner;
    RelationMode mode;
    Vec<ItemRef> targets;
};
struct LegacyRelationInput {
    i64       owner_id;
    i64       artist_id;
    Timestamp update_at;
};
} // namespace qcm::backend

namespace qcm::backend
{
auto relation_contains(slice<i64> ids, i64 id) -> bool {
    for (const auto& value : ids)
        if (value == id) return true;
    return false;
}
template<class Model, auto OwnerMember>
auto relation_upsert() {
    return relation_query::try_compile([](relation_query::QueryBuilder& b) {
        auto table = b.use(database_tables).table<Model>();
        return b
            .insert_into(table.source(),
                         table.template col<OwnerMember>(),
                         table.template col<&Model::artist_id>(),
                         table.template col<&Model::update_at>())
            .values(b.param<i64>("owner"_str),
                    b.param<i64>("artist"_str),
                    b.param<Timestamp>("now"_str))
            .on_conflict_do_update(
                array<relation_query::Expr, 2> { table.template col<OwnerMember>(),
                                                 table.template col<&Model::artist_id>() }
                    .as_slice(),
                array<relation_query::Expr, 1> {
                    table.template col<&Model::update_at>().set(
                        b.excluded(table.template col<&Model::update_at>())) }
                    .as_slice());
    });
}
template<class Model, auto OwnerMember, ItemType OwnerType>
auto write_legacy_relations(sqlito::Transaction& tx, i64 provider,
                            slice<LegacyRelationInput> entries) -> DbWriteResult<empty> {
    auto               query = rstd_try((relation_upsert<Model, OwnerMember>()));
    auto               stmt  = rstd_try(liorm::sqlito::prepare(tx, query));
    array<ItemType, 1> owner_expected { OwnerType };
    array<ItemType, 1> artist_expected { ItemType::Artist };
    for (const auto& entry : entries) {
        rstd_try(resolve(tx, provider, ItemRef::Id(entry.owner_id), owner_expected.as_slice()));
        rstd_try(resolve(tx, provider, ItemRef::Id(entry.artist_id), artist_expected.as_slice()));
        rstd_try(stmt.bind("owner"_str, entry.owner_id));
        rstd_try(stmt.bind("artist"_str, entry.artist_id));
        rstd_try(stmt.bind("now"_str, entry.update_at));
        rstd_try(liorm::sqlito::discard(stmt));
        rstd_try(stmt.reset());
    }
    return Ok(empty {});
}
template<class Model, auto OwnerMember, ItemType OwnerType>
auto write_relations(sqlito::Transaction& tx, i64 provider, slice<RelationInput> entries)
    -> DbWriteResult<empty> {
    array<ItemType, 1> owner_expected { OwnerType };
    auto     remove = rstd_try(relation_query::try_compile([](relation_query::QueryBuilder& b) {
        auto table = b.use(database_tables).table<Model>();
        return b.delete_from(table.source())
            .where(table.template col<OwnerMember>() == b.param<i64>("owner"_str));
    }));
    auto     upsert = rstd_try((relation_upsert<Model, OwnerMember>()));
    Vec<i64> owners;
    for (const auto& entry : entries) {
        auto owner_item = rstd_try(resolve(tx, provider, entry.owner, owner_expected.as_slice()));
        if (relation_contains(owners.as_slice(), owner_item.id))
            return Err(DbWriteError::Invalid(String::make("duplicate relation owner"_str)));
        owners.push(i64(owner_item.id));
        Vec<i64> targets;
        for (const auto& reference : entry.targets.as_slice()) {
            array<ItemType, 1> artist_expected { ItemType::Artist };
            auto               artist_item =
                rstd_try(resolve(tx, provider, reference, artist_expected.as_slice()));
            if (artist_item.library_id != owner_item.library_id)
                return Err(
                    DbWriteError::Invalid(String::make("artist relation crosses libraries"_str)));
            if (relation_contains(targets.as_slice(), artist_item.id))
                return Err(DbWriteError::Invalid(String::make("duplicate relation target"_str)));
            targets.push(i64(artist_item.id));
        }
        if (entry.mode == RelationMode::Replace) {
            auto stmt = rstd_try(liorm::sqlito::prepare(tx, remove));
            rstd_try(stmt.bind("owner"_str, owner_item.id));
            rstd_try(liorm::sqlito::discard(stmt));
        }
        auto now = Timestamp::now();
        for (const auto& artist : targets.as_slice()) {
            auto stmt = rstd_try(liorm::sqlito::prepare(tx, upsert));
            rstd_try(stmt.bind("owner"_str, owner_item.id));
            rstd_try(stmt.bind("artist"_str, artist));
            rstd_try(stmt.bind("now"_str, now));
            rstd_try(liorm::sqlito::discard(stmt));
        }
    }
    return Ok(empty {});
}
} // namespace qcm::backend

export namespace qcm::backend
{
auto write_legacy_album_artists(sqlito::Transaction& tx, i64 provider,
                                slice<LegacyRelationInput> entries) -> DbWriteResult<empty> {
    return write_legacy_relations<RelAlbumArtist, &RelAlbumArtist::album_id, ItemType::Album>(
        tx, provider, entries);
}
auto write_legacy_song_artists(sqlito::Transaction& tx, i64 provider,
                               slice<LegacyRelationInput> entries) -> DbWriteResult<empty> {
    return write_legacy_relations<RelSongArtist, &RelSongArtist::song_id, ItemType::Song>(
        tx, provider, entries);
}
// The caller owns the transaction and runs relations after their owners exist.
auto write_album_artists(sqlito::Transaction& tx, i64 provider, slice<RelationInput> entries)
    -> DbWriteResult<empty> {
    return write_relations<RelAlbumArtist, &RelAlbumArtist::album_id, ItemType::Album>(
        tx, provider, entries);
}
auto write_song_artists(sqlito::Transaction& tx, i64 provider, slice<RelationInput> entries)
    -> DbWriteResult<empty> {
    return write_relations<RelSongArtist, &RelSongArtist::song_id, ItemType::Song>(
        tx, provider, entries);
}
} // namespace qcm::backend
