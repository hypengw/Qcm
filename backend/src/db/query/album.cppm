module;
#include <rstd/macro.hpp>
export module qcm.backend:db.query.album;
export import :db.query.input;
export import :db.error.query;
export import sqlito;
import liorm;
import :db.query.album_filters;
import :model.album;
import :model.dynamic;
import :model.song;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace q = liorm::query;

export namespace qcm::backend
{
auto album_exists(sqlito::Connection& db, i64 id) -> QueryResult<bool>;
auto album_song_ids(sqlito::Connection& db, i64 id) -> QueryResult<Vec<i64>>;
auto album_exists(sqlito::Transaction& db, i64 id) -> QueryResult<bool>;
auto album_song_ids(sqlito::Transaction& db, i64 id) -> QueryResult<Vec<i64>>;

struct AlbumQueryPage {
    Vec<i64> ids;
    i64      total {};
};

template<class AlbumTable, class DynamicTable>
auto album_order(q::QueryBuilder& b, const AlbumTable& album, const DynamicTable& dynamic,
                 AlbumSort sort) -> q::Expr {
    switch (sort) {
    case AlbumSort::SortTitle: return album.template col<&Album::sort_name>();
    case AlbumSort::Year:
    case AlbumSort::PublishTime: return album.template col<&Album::publish_time>();
    case AlbumSort::TrackCount: return album.template col<&Album::track_count>();
    case AlbumSort::AddedTime: return album.template col<&Album::added_at>();
    case AlbumSort::DiscCount: return album.template col<&Album::disc_count>();
    case AlbumSort::LastPlayedAt: return dynamic.template col<&Dynamic::last_played_at>();
    case AlbumSort::Random: return b.function(q::Function::Random);
    case AlbumSort::Title: return album.template col<&Album::name>();
    }
    return album.template col<&Album::name>();
}

template<class Prepared>
auto bind_album_parameters(Prepared& stmt, slice<i64> libraries,
                           const AlbumFilterExpressions& filters) -> QueryResult<empty> {
    rstd_try(stmt.bind_group("libraries"_str, libraries));
    rstd_try(stmt.bind("external"_str, false));
    return filters.bind(stmt);
}

template<class Database>
auto album_query_page(Database& db, const AlbumQuery& request) -> QueryResult<AlbumQueryPage>;

template<class SongTable>
auto album_song_order(q::QueryBuilder& b, const SongTable& song, SongSort sort) -> q::Expr {
    switch (sort) {
    case SongSort::TrackNumber: return song.template col<&Song::track_number>();
    case SongSort::SortTitle: return song.template col<&Song::sort_name>();
    case SongSort::PublishTime: return song.template col<&Song::publish_time>();
    case SongSort::Duration: return song.template col<&Song::duration>();
    case SongSort::Popularity: return song.template col<&Song::popularity>();
    case SongSort::Random: return b.function(q::Function::Random);
    case SongSort::Title: return song.template col<&Song::name>();
    }
    return song.template col<&Song::name>();
}

template<class Database>
auto album_song_ids(Database& db, const AlbumSongQuery& request) -> QueryResult<Vec<i64>>;

extern template auto album_query_page(sqlito::Connection&, const AlbumQuery&)
    -> QueryResult<AlbumQueryPage>;
extern template auto album_query_page(sqlito::Transaction&, const AlbumQuery&)
    -> QueryResult<AlbumQueryPage>;
extern template auto album_song_ids(sqlito::Connection&, const AlbumSongQuery&)
    -> QueryResult<Vec<i64>>;
extern template auto album_song_ids(sqlito::Transaction&, const AlbumSongQuery&)
    -> QueryResult<Vec<i64>>;
} // namespace qcm::backend
