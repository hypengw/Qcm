module;
#include <rstd/macro.hpp>
export module qcm.backend:db.query.album;
export import :db.query.input;
export import :db.error.query;
export import sqlito;
import liorm;
import liorm.sqlito;
import :db.query.album_filters;
import :db.schema;
import :model.album;
import :model.item;
import :model.dynamic;
import :model.song;
import :pagination;
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
auto album_query_page(Database& db, const AlbumQuery& request) -> QueryResult<AlbumQueryPage> {
    auto                   libraries = request.libraries.as_slice();
    q::QueryBuilder        b;
    auto                   schema      = b.use(database_tables);
    auto                   album       = schema.table<Album>();
    auto                   item        = schema.table<Item>();
    auto                   dynamic     = schema.table<Dynamic>();
    auto                   library_ids = b.param_group<i64>("libraries"_str, libraries.len());
    AlbumFilterExpressions filters(b);
    auto condition = item.col<&Item::library_id>().in(library_ids) &&
                     (dynamic.col<&Dynamic::is_external>() == b.param<bool>("external"_str));
    auto extra     = filters.build(schema, album, dynamic, request.filters);
    if (extra) condition = condition && *extra;
    auto statement =
        b.select(album.col<&Album::id>())
            .from(album.source())
            .join(item.source(), b.on(schema.relation(db_relations::album_item), album, item))
            .left_join(dynamic.source(),
                       b.on(schema.relation(db_relations::dynamic_album), dynamic, album))
            .where(condition);
    auto order = album_order(b, album, dynamic, request.sort);
    statement  = statement.order_by(request.ascending ? order.asc() : order.desc(),
                                    album.col<&Album::id>().asc());
    auto query = rstd_try(
        q::try_compile_page(statement, i64(request.page.size.to_primitive()), request.page.offset));
    auto stmt = rstd_try(liorm::sqlito::prepare_page(db, rstd::move(query)));
    rstd_try(bind_album_parameters(stmt, libraries, filters));
    auto rows =
        rstd_try(stmt.fetch(QueryIdProjection { "album_query returned a null ID or count"_str }));
    AlbumQueryPage result;
    result.total = rows.total;
    result.ids   = rstd::move(rows.rows);
    return Ok(rstd::move(result));
}

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
auto album_song_ids(Database& db, const AlbumSongQuery& request) -> QueryResult<Vec<i64>> {
    auto                   libraries = request.albums.libraries.as_slice();
    q::QueryBuilder        b;
    auto                   schema      = b.use(database_tables);
    auto                   album       = schema.table<Album>();
    auto                   item        = schema.table<Item>();
    auto                   dynamic     = schema.table<Dynamic>();
    auto                   song        = schema.table<Song>();
    auto                   library_ids = b.param_group<i64>("libraries"_str, libraries.len());
    AlbumFilterExpressions filters(b);
    auto condition = item.col<&Item::library_id>().in(library_ids) &&
                     (dynamic.col<&Dynamic::is_external>() == b.param<bool>("external"_str));
    auto extra     = filters.build(schema, album, dynamic, request.albums.filters);
    if (extra) condition = condition && *extra;
    auto albums =
        b.select(album.col<&Album::id>().as("sorted_album_id"_str),
                 b.row_number(album_order(b, album, dynamic, request.albums.sort))
                     .as("sorted_album_ord"_str))
            .from(album.source())
            .join(item.source(), b.on(schema.relation(db_relations::album_item), album, item))
            .join(dynamic.source(),
                  b.on(schema.relation(db_relations::dynamic_album), dynamic, album))
            .where(condition);
    auto album_id  = b.col("sorted_album_ids"_str, "sorted_album_id"_str);
    auto album_ord = b.col("sorted_album_ids"_str, "sorted_album_ord"_str);
    auto order     = album_song_order(b, song, request.sort);
    auto statement =
        b.select(song.col<&Song::id>())
            .from(song.source())
            .join(b.cte("sorted_album_ids"_str), song.col<&Song::album_id>() == album_id)
            .order_by(request.albums.ascending ? album_ord.asc() : album_ord.desc(),
                      request.ascending ? order.asc() : order.desc(),
                      song.col<&Song::id>().asc())
            .with("sorted_album_ids"_str, albums);
    auto query = rstd_try(q::try_compile_expanded_planned(statement));
    auto stmt  = rstd_try(liorm::sqlito::prepare(db, rstd::move(query)));
    rstd_try(bind_album_parameters(stmt, libraries, filters));
    return liorm::sqlito::all_rows(
        stmt, QueryIdProjection { "album_query returned a null ID or count"_str });
}
} // namespace qcm::backend
