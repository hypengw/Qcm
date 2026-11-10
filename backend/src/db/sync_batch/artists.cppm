module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:db.sync_batch.artists;
export import :db.sync_batch.item_reference;
export import :model.artist;
import liorm.sqlito;
import :db.codec;
import :db.schema;
import :db.error.query;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
[[pmacro::invoke("qcm::entity_input", ArtistInput, ArtistFields, Artist, {
    name: String,
    sort_name: Option<String>,
    description: String,
    album_count: i32,
    music_count: i32,
    added_at: Option<Timestamp>,
})]];
} // namespace qcm::backend

namespace qcm::backend
{
template<class Database>
auto artist_exists(Database& db, i64 id) -> QueryResult<bool> {
    return Ok(rstd_try((liorm::sqlito::exists<database_tables, Artist>(db, id))));
}
auto find_artist(sqlito::Transaction& tx, i64 id) -> DbWriteResult<Option<Artist>> {
    return Ok(rstd_try((liorm::sqlito::find<database_tables, Artist>(tx, id))));
}
} // namespace qcm::backend

export namespace qcm::backend
{
auto write_legacy_artists(sqlito::Transaction& tx, i64 provider, slice<Artist> entries)
    -> DbWriteResult<Vec<i64>> {
    array<ItemType, 1> expected { ItemType::Artist };
    Vec<i64>           ids;
    for (const auto& entry : entries) {
        rstd_try(resolve(tx, provider, ItemRef::Id(entry.id), expected.as_slice()));
        auto   old = rstd_try(find_artist(tx, entry.id));
        Artist model { entry.id,
                       entry.name.clone(),
                       entry.sort_name.clone(),
                       entry.description.clone(),
                       entry.album_count,
                       entry.music_count,
                       entry.added_at };
        rstd_try(old.is_none() ? liorm::sqlito::insert_all<database_tables>(tx, rstd::move(model))
                               : liorm::sqlito::update_all<database_tables>(tx, rstd::move(model)));
        ids.push(i64(entry.id));
    }
    return Ok(rstd::move(ids));
}
// The caller owns the transaction and the mask prevalidation for empty groups.
auto write_artists(sqlito::Transaction& tx, i64 provider, slice<ArtistInput> entries,
                   const Mask& mask) -> DbWriteResult<Vec<Artist>> {
    array<ItemType, 1> expected { ItemType::Artist };
    Vec<Artist>        result;
    for (const auto& entry : entries) {
        auto item     = rstd_try(resolve(tx, provider, entry.item, expected.as_slice()));
        auto old      = rstd_try(find_artist(tx, item.id));
        auto inserted = old.is_none();
        auto model    = inserted ? Artist { item.id, String::make(), None(), String::make(),
                                            i32(),   i32(),          None() }
                                 : rstd::move(old).unwrap();
        rstd_try(entry.apply(model, inserted, mask));
        if (model.name.is_empty())
            return Err(DbWriteError::Invalid(String::make("entity name is required"_str)));
        auto written =
            rstd_try(inserted ? liorm::sqlito::insert_all<database_tables>(tx, rstd::move(model))
                              : liorm::sqlito::update_all<database_tables>(tx, rstd::move(model)));
        result.push(rstd::move(written));
    }
    for (const auto& artist : result.as_slice())
        if (artist.album_count < i32() || artist.music_count < i32())
            return Err(DbWriteError::Invalid(String::make("negative artist quantity"_str)));
    return Ok(rstd::move(result));
}
} // namespace qcm::backend
