module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.query.artist_albums;
import :db.query.album;
import :db.query.artist;
import :db.schema;
import :model.album;
import :model.dynamic;
import :model.rel_album_artist;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace artist_album_query = liorm::query;

namespace qcm::backend
{
namespace
{
inline constexpr liorm::TupleProjection<Option<i64>> artist_album_id_projection;
} // namespace

template<class Database>
auto artist_album_page(Database& db, ArtistAlbumQuery input) -> QueryResult<Option<IdQueryPage>> {
    if (! rstd_try(artist_exists(db, input.artist_id))) return Ok(None<IdQueryPage>());
    namespace q = artist_album_query;
    q::QueryBuilder b;
    auto            schema   = b.use(database_tables);
    auto            album    = schema.table<Album>();
    auto            relation = schema.table<RelAlbumArtist>();
    auto            dynamic  = schema.table<Dynamic>();
    auto condition = relation.col<&RelAlbumArtist::artist_id>() == b.param<i64>("artist"_str);
    auto order     = album_order(b, album, dynamic, input.sort);
    auto select =
        artist_album_id_projection.select(b, album.col<&Album::id>())
            .from(album.source())
            .left_join(dynamic.source(),
                       b.on(schema.relation(db_relations::dynamic_album), dynamic, album))
            .join(relation.source(),
                  b.on(schema.relation(db_relations::rel_album_artist_album), relation, album))
            .where(condition)
            .order_by(input.ascending ? order.asc() : order.desc());
    auto query = rstd_try(
        q::try_compile_page(select, i64(input.page.size.to_primitive()), input.page.offset));
    auto stmt = rstd_try(liorm::sqlito::prepare_page(db, rstd::move(query)));
    rstd_try(stmt.bind("artist"_str, input.artist_id));
    auto        rows = rstd_try(stmt.fetch(artist_album_id_projection));
    IdQueryPage result;
    result.total = rows.total;
    for (auto& row : rows.rows) {
        result.ids.push(rstd::get<0>(row).unwrap_or(i64()));
    }
    return Ok(Some(rstd::move(result)));
}

template auto artist_album_page(sqlito::Connection&, ArtistAlbumQuery)
    -> QueryResult<Option<IdQueryPage>>;
template auto artist_album_page(sqlito::Transaction&, ArtistAlbumQuery)
    -> QueryResult<Option<IdQueryPage>>;
} // namespace qcm::backend
