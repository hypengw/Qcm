module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.query.album;
import :db.codec;
import :db.schema;
import :model.album;
import :model.song;
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
} // namespace qcm::backend
