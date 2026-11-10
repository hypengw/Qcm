module;
#include <rstd/macro.hpp>
export module qcm.backend:db.sync.retention;
export import :db.sync.run;
import :db.schema;
import :model.item;
import :model.song;
import :model.rel_song_artist;
import :model.rel_album_artist;
import liorm.sqlito;
using rstd::collections::HashSet;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace retention_query = liorm::query;

namespace qcm::backend
{
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
} // namespace qcm::backend

export namespace qcm::backend
{
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
