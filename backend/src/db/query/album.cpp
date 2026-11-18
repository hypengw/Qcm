module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.query.album;
import :db.codec;
import :db.schema;
import :model.album;
import :model.song;
import :db.query.album_filters;
import :model.dynamic;
import :model.item;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace album_query = liorm::query;

namespace qcm::backend
{
namespace
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
auto album_exists_impl(Database& db, i64 id) -> QueryResult<bool> {
    return Ok(rstd_try((liorm::sqlito::exists<database_tables, Album>(db, id))));
}
template<class Database>
auto album_song_ids_impl(Database& db, i64 album_id) -> QueryResult<Vec<i64>> {
    auto     rows = rstd_try(liorm::sqlito::fetch_all<album_song_ids_query>(
        db, album_song_id_projection, liorm::parameters(liorm::param<"album_id">(album_id))));
    Vec<i64> result;
    for (auto& row : rows) result.push(rstd::move(rstd::get<0>(row)));
    return Ok(rstd::move(result));
}
} // namespace

auto album_exists(sqlito::Connection& db, i64 id) -> QueryResult<bool> {
    return album_exists_impl(db, id);
}

auto album_song_ids(sqlito::Connection& db, i64 id) -> QueryResult<Vec<i64>> {
    return album_song_ids_impl(db, id);
}

auto album_exists(sqlito::Transaction& db, i64 id) -> QueryResult<bool> {
    return album_exists_impl(db, id);
}

auto album_song_ids(sqlito::Transaction& db, i64 id) -> QueryResult<Vec<i64>> {
    return album_song_ids_impl(db, id);
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

template auto album_query_page(sqlito::Connection&, const AlbumQuery&)
    -> QueryResult<AlbumQueryPage>;
template auto album_query_page(sqlito::Transaction&, const AlbumQuery&)
    -> QueryResult<AlbumQueryPage>;
template auto album_song_ids(sqlito::Connection&, const AlbumSongQuery&) -> QueryResult<Vec<i64>>;
template auto album_song_ids(sqlito::Transaction&, const AlbumSongQuery&) -> QueryResult<Vec<i64>>;
} // namespace qcm::backend
