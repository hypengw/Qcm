module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.query.song;
import :db.query.album;
import :db.schema;
import :model.item;
import :model.song;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
template<class Database>
auto filtered_song_ids(Database& db, const FilteredSongQuery& request) -> QueryResult<Vec<i64>> {
    q::QueryBuilder       b;
    auto                  schema = b.use(database_tables);
    auto                  song   = schema.table<Song>();
    auto                  item   = schema.table<Item>();
    SongFilterExpressions filters(b);
    auto                  condition = filters.build(schema, song, request.filters);
    auto                  statement =
        b.select(song.col<&Song::id>())
            .from(song.source())
            .join(item.source(), b.on(schema.relation(db_relations::song_item), song, item));
    if (condition) statement = statement.where(*condition);
    auto order = album_song_order(b, song, request.sort);
    auto id    = song.col<&Song::id>();
    if (request.sort == SongSort::TrackNumber) {
        auto disc = song.col<&Song::disc_number>();
        statement = statement.order_by(request.ascending ? disc.asc() : disc.desc(),
                                       request.ascending ? order.asc() : order.desc(),
                                       request.ascending ? id.asc() : id.desc());
    } else {
        statement = statement.order_by(request.ascending ? order.asc() : order.desc(),
                                       request.ascending ? id.asc() : id.desc());
    }
    auto query = rstd_try(q::try_compile(statement));
    auto stmt  = rstd_try(liorm::sqlito::prepare(db, rstd::move(query)));
    rstd_try(filters.bind(stmt));
    return liorm::sqlito::all_rows(
        stmt, QueryIdProjection { "song_query returned a null ID or count"_str });
}

template auto filtered_song_ids(sqlito::Connection&, const FilteredSongQuery&)
    -> QueryResult<Vec<i64>>;
template auto filtered_song_ids(sqlito::Transaction&, const FilteredSongQuery&)
    -> QueryResult<Vec<i64>>;
} // namespace qcm::backend
