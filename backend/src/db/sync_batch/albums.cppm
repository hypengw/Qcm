module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:db.sync_batch.albums;
export import :db.sync_batch.item_reference;
export import :model.album;
import liorm.sqlito;
import :db.codec;
import :db.schema;
import :db.error.query;
import :model.song;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace album_query = liorm::query;

export namespace qcm::backend
{
[[pmacro::invoke("qcm::entity_input", AlbumInput, AlbumFields, Album, {
    name: String,
    sort_name: Option<String>,
    track_count: i32,
    disc_count: i32,
    type: AlbumType,
    duration: i64,
    language: Option<String>,
    description: Option<String>,
    company: Option<String>,
    publish_time: Option<Timestamp>,
    added_at: Option<Timestamp>,
})]];
} // namespace qcm::backend

namespace qcm::backend
{
inline constexpr liorm::TupleProjection<i64> album_song_id_projection;

inline constexpr auto album_song_ids_query = album_query::compile([](album_query::QueryBuilder& b) {
    auto song = b.use(database_tables).table<Song>();
    return album_song_id_projection.select(b, song.col<&Song::id>())
        .from(song.source())
        .where(song.col<&Song::album_id>() == b.param<i64>("album_id"_str))
        .order_by(song.col<&Song::disc_number>().asc(),
                  song.col<&Song::track_number>().asc(),
                  song.col<&Song::id>().asc());
});
template<class Database>
auto album_exists(Database& db, i64 id) -> QueryResult<bool> {
    return Ok(rstd_try((liorm::sqlito::exists<database_tables, Album>(db, id))));
}
template<class Database>
auto album_song_ids(Database& db, i64 album_id) -> QueryResult<Vec<i64>> {
    auto     rows = rstd_try(liorm::sqlito::fetch_all<album_song_ids_query>(
        db, album_song_id_projection, liorm::parameters(liorm::param<"album_id">(album_id))));
    Vec<i64> result;
    for (auto& row : rows) result.push(rstd::move(rstd::get<0>(row)));
    return Ok(rstd::move(result));
}
auto find_album(sqlito::Transaction& tx, i64 id) -> DbWriteResult<Option<Album>> {
    return Ok(rstd_try((liorm::sqlito::find<database_tables, Album>(tx, id))));
}
} // namespace qcm::backend

export namespace qcm::backend
{
auto write_legacy_albums(sqlito::Transaction& tx, i64 provider, slice<Album> entries,
                         const Mask& mask) -> DbWriteResult<Vec<i64>> {
    rstd_try(mask.validate<AlbumInput>());
    const auto&        selected = mask;
    array<ItemType, 1> expected { ItemType::Album };
    Vec<i64>           ids;
    for (const auto& entry : entries) {
        rstd_try(resolve(tx, provider, ItemRef::Id(entry.id), expected.as_slice()));
        auto old = rstd_try(find_album(tx, entry.id));
        if (old.is_none()) {
            Album model { entry.id,
                          entry.name.clone(),
                          entry.sort_name.clone(),
                          entry.track_count,
                          entry.disc_count,
                          entry.type,
                          entry.duration,
                          entry.language.clone(),
                          entry.description.clone(),
                          entry.company.clone(),
                          entry.publish_time,
                          entry.added_at };
            rstd_try(liorm::sqlito::insert_all<database_tables>(tx, rstd::move(model)));
        } else {
            liorm::Changeset<Album> changes;
            sync_schema::Definition<AlbumFields>::apply_changes(changes, entry, selected);
            if (! changes.empty())
                rstd_try(liorm::sqlito::update<database_tables>(tx, entry.id, rstd::move(changes)));
        }
        ids.push(i64(entry.id));
    }
    return Ok(rstd::move(ids));
}
// The caller owns the transaction and the mask prevalidation for empty groups.
auto write_albums(sqlito::Transaction& tx, i64 provider, slice<AlbumInput> entries,
                  const Mask& mask) -> DbWriteResult<Vec<Album>> {
    array<ItemType, 1> expected { ItemType::Album };
    Vec<Album>         result;
    for (const auto& entry : entries) {
        auto item     = rstd_try(resolve(tx, provider, entry.item, expected.as_slice()));
        auto old      = rstd_try(find_album(tx, item.id));
        auto inserted = old.is_none();
        auto model =
            inserted ? Album { item.id, String::make(), None(), i32(),  i32(),  AlbumType::Album,
                               i64(),   None(),         None(), None(), None(), None() }
                     : rstd::move(old).unwrap();
        rstd_try(entry.apply(model, inserted, mask));
        if (model.name.is_empty())
            return Err(DbWriteError::Invalid(String::make("entity name is required"_str)));
        auto written =
            rstd_try(inserted ? liorm::sqlito::insert_all<database_tables>(tx, rstd::move(model))
                              : liorm::sqlito::update_all<database_tables>(tx, rstd::move(model)));
        result.push(rstd::move(written));
    }
    for (const auto& album : result.as_slice())
        if (album.duration < i64() || album.track_count < i32() || album.disc_count < i32())
            return Err(DbWriteError::Invalid(String::make("negative album quantity"_str)));
    return Ok(rstd::move(result));
}
} // namespace qcm::backend
