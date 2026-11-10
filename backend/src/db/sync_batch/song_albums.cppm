module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:db.sync_batch.song_albums;
export import :db.sync_batch.songs;
export import :db.sync_batch.item_reference;
import liorm.sqlito;
import :db.schema;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace q = liorm::query;

export namespace qcm::backend
{
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", decode_only)]]
SongAlbumInput {
    ItemRef song;
    [[pmacro::helper("serde::derive", default)]]
    Option<ItemRef> album;
};
struct LegacySongAlbumInput {
    String song_native_id;
    String album_native_id;
};
} // namespace qcm::backend

namespace qcm::backend
{
auto song_album_contains(slice<i64> ids, i64 id) -> bool {
    for (const auto& value : ids)
        if (value == id) return true;
    return false;
}
auto song_album_item(sqlito::Transaction& tx, i64 provider, const ItemRef& reference)
    -> DbWriteResult<Item> {
    array<ItemType, 1> expected { ItemType::Song };
    return resolve(tx, provider, reference, expected.as_slice());
}
auto song_album_album(sqlito::Transaction& tx, i64 provider, const Option<ItemRef>& album,
                      const Item& song) -> DbWriteResult<Option<i64>> {
    if (album.is_none()) return Ok<Option<i64>>(None());
    array<ItemType, 1> expected { ItemType::Album };
    auto               model = rstd_try(resolve(tx, provider, *album, expected.as_slice()));
    if (song.library_id != model.library_id)
        return Err(DbWriteError::Invalid(String::make("song and album must share a library"_str)));
    return Ok(Some(model.id));
}
auto song_album_update(sqlito::Transaction& tx, i64 id, Option<i64> album_id)
    -> DbWriteResult<empty> {
    using C = liorm::Columns<Song>;
    liorm::Changeset<Song> changes { C::album_id = rstd::move(album_id) };
    rstd_try(liorm::sqlito::update<database_tables>(tx, id, rstd::move(changes)));
    return Ok(empty {});
}
} // namespace qcm::backend

export namespace qcm::backend
{
auto write_legacy_song_album_ids(sqlito::Transaction& tx, i64 provider, i64 library,
                                 slice<LegacySongAlbumInput> entries)
    -> Result<empty, DbWriteError> {
    rstd_try(validate_library(tx, provider, library));
    if (entries.is_empty()) return Ok(empty {});
    auto query = rstd_try(q::try_compile_expanded_planned([&](q::QueryBuilder& b) {
        auto schema = b.use(database_tables);
        auto item   = schema.table<Item>();
        auto album  = schema.table<Item>("album_item"_str);
        auto song   = schema.table<Song>();
        auto inputs = b.values_groups(b.param_group<q::TextParameter>("songs"_str, entries.len()),
                                      b.param_group<q::TextParameter>("albums"_str, entries.len()));
        auto native_ids = b.select(b.col("input_ids"_str, "column1"_str).as("input_col1"_str),
                                   b.col("input_ids"_str, "column2"_str).as("input_col2"_str))
                              .from(inputs.as("input_ids"_str));
        auto relations =
            b.select(item.col<&Item::id>().as("id"_str), album.col<&Item::id>().as("album_id"_str))
                .from(item.source())
                .join(b.cte("native_id_map"_str),
                      item.col<&Item::native_id>() == b.col("native_id_map"_str, "input_col1"_str))
                .join(album.source(),
                      (album.col<&Item::library_id>() == b.param<i64>("library"_str)) &&
                          (album.col<&Item::type>() ==
                           b.integer(static_cast<int>(ItemType::Album))) &&
                          (album.col<&Item::native_id>() ==
                           b.col("native_id_map"_str, "input_col2"_str)))
                .where((item.col<&Item::library_id>() == b.param<i64>("library"_str)) &&
                       (item.col<&Item::type>() == b.integer(static_cast<int>(ItemType::Song))));
        return b.update(song.source())
            .set(song.col<&Song::album_id>().set(b.col("res"_str, "album_id"_str)))
            .from(b.cte("res"_str))
            .where(b.col("res"_str, "id"_str) == song.col<&Song::id>())
            .with("native_id_map"_str, native_ids)
            .with("res"_str, relations);
    }));
    auto stmt  = rstd_try(liorm::sqlito::prepare(tx, rstd::move(query)));
    rstd_try(stmt.bind("library"_str, library));
    rstd_try(
        stmt.bind_group_with("songs"_str, entries.len(), [entries](usize index) -> const String& {
            return entries[index].song_native_id;
        }));
    rstd_try(
        stmt.bind_group_with("albums"_str, entries.len(), [entries](usize index) -> const String& {
            return entries[index].album_native_id;
        }));
    // Keep every input row: UPDATE FROM chooses a match for duplicate songs.
    rstd_try(liorm::sqlito::discard(stmt));
    return Ok(empty {});
}
// The caller owns the transaction and the song rows written earlier in the batch.
auto write_song_albums(sqlito::Transaction& tx, i64 provider, slice<SongInput> songs,
                       slice<SongAlbumInput> relations, bool songs_allow_album_id,
                       slice<i64> new_songs) -> DbWriteResult<empty> {
    Vec<i64> handled;
    for (const auto& entry : songs) {
        if (entry.album_id.is_missing()) continue;
        auto song = rstd_try(song_album_item(tx, provider, entry.item));
        if (! song_album_contains(new_songs, song.id) && ! songs_allow_album_id) continue;
        handled.push(i64(song.id));
        auto album = rstd_try(song_album_album(tx, provider, entry.album_id.value(), song));
        rstd_try(song_album_update(tx, song.id, rstd::move(album)));
    }
    for (const auto& entry : relations) {
        auto song = rstd_try(song_album_item(tx, provider, entry.song));
        if (song_album_contains(handled.as_slice(), song.id))
            return Err(DbWriteError::Invalid(String::make("duplicate song album relation"_str)));
        handled.push(i64(song.id));
        auto album = rstd_try(song_album_album(tx, provider, entry.album, song));
        rstd_try(song_album_update(tx, song.id, rstd::move(album)));
    }
    return Ok(empty {});
}
} // namespace qcm::backend
