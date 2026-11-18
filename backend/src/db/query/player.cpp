module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.query.player;
import :db.error.process;
import :db.schema;
import :model.song;
import :model.album;
import :model.artist;
import :model.rel_song_artist;
import :model.rel_album_artist;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace player_query = liorm::query;

namespace qcm::backend
{
struct PlayerTrackFields {
    String         title;
    Option<String> album;
    Option<i64>    album_id;
    i64            duration;
    bool           can_play;
};
struct PlayerArtistField {
    String name;
};
inline constexpr liorm::RecordProjection<PlayerTrackFields, &PlayerTrackFields::title,
                                         &PlayerTrackFields::album, &PlayerTrackFields::album_id,
                                         &PlayerTrackFields::duration, &PlayerTrackFields::can_play>
    player_track_projection;
inline constexpr liorm::RecordProjection<PlayerArtistField, &PlayerArtistField::name>
                      player_artist_projection;
inline constexpr auto player_track_query = player_query::compile([](player_query::QueryBuilder& b) {
    auto schema = b.use(database_tables);
    auto song   = schema.table<Song>();
    auto album  = schema.table<Album>();
    return player_track_projection
        .select(b,
                song.col<&Song::name>(),
                album.col<&Album::name>(),
                song.col<&Song::album_id>(),
                song.col<&Song::duration>(),
                song.col<&Song::can_play>())
        .from(song.source())
        .left_join(album.source(), song.col<&Song::album_id>() == album.col<&Album::id>())
        .where(song.col<&Song::id>() == b.param<i64>("id"_str))
        .limit(b.integer(1));
});
template<bool AlbumArtists>
inline constexpr auto player_artists_query =
    player_query::compile([](player_query::QueryBuilder& b) {
        auto schema = b.use(database_tables);
        auto artist = schema.table<Artist>();
        if constexpr (AlbumArtists) {
            auto relation = schema.table<RelAlbumArtist>();
            return player_artist_projection.select(b, artist.col<&Artist::name>())
                .from(artist.source())
                .join(relation.source(),
                      relation.col<&RelAlbumArtist::artist_id>() == artist.col<&Artist::id>())
                .where(relation.col<&RelAlbumArtist::album_id>() == b.param<i64>("id"_str))
                .order_by(artist.col<&Artist::id>().asc());
        } else {
            auto relation = schema.table<RelSongArtist>();
            return player_artist_projection.select(b, artist.col<&Artist::name>())
                .from(artist.source())
                .join(relation.source(),
                      relation.col<&RelSongArtist::artist_id>() == artist.col<&Artist::id>())
                .where(relation.col<&RelSongArtist::song_id>() == b.param<i64>("id"_str))
                .order_by(artist.col<&Artist::id>().asc());
        }
    });
template<bool AlbumArtists>
auto player_artist_names(sqlito::Connection& db, i64 id) -> ProcessResult<Vec<String>> {
    auto        rows = rstd_try(liorm::sqlito::fetch_all<player_artists_query<AlbumArtists>>(
        db, player_artist_projection, liorm::parameters(liorm::param<"id">(id))));
    Vec<String> names;
    for (auto& row : rows) names.push(rstd::move(row.name));
    return Ok(rstd::move(names));
}
} // namespace qcm::backend

namespace qcm::backend
{
auto player_track(sqlito::Connection& db, i64 id) -> ProcessResult<PlayerTrack> {
    auto row = rstd_try(liorm::sqlito::fetch_one<player_track_query>(
        db, player_track_projection, liorm::parameters(liorm::param<"id">(id))));
    if (row.is_none()) return Err(ProcessError::NoSuchSong(rstd::format("{}", id)));
    auto        artists = rstd_try(player_artist_names<false>(db, id));
    Vec<String> album_artists;
    if (row->album_id.is_some())
        album_artists = rstd_try(player_artist_names<true>(db, *row->album_id));
    return Ok(PlayerTrack { rstd::move(row->title),
                            row->album.unwrap_or(String::make()),
                            rstd::move(artists),
                            rstd::move(album_artists),
                            row->duration,
                            row->can_play });
}
} // namespace qcm::backend
