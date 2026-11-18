module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.query.artist;
import :db.codec;
import :db.schema;
import :model.artist;
import :db.query.filter_expressions;
import :model.item;
import :model.rel_album_artist;
import :model.rel_song_artist;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
namespace
{
template<class Database>
auto artist_exists_impl(Database& db, i64 id) -> QueryResult<bool> {
    return Ok(rstd_try((liorm::sqlito::exists<database_tables, Artist>(db, id))));
}
} // namespace

auto artist_exists(sqlito::Connection& db, i64 id) -> QueryResult<bool> {
    return artist_exists_impl(db, id);
}

auto artist_exists(sqlito::Transaction& db, i64 id) -> QueryResult<bool> {
    return artist_exists_impl(db, id);
}

template<class Database>
auto artist_query_page(Database& db, const ArtistQuery& request, bool album_artists)
    -> QueryResult<ArtistQueryPage> {
    auto                    libraries = request.libraries.as_slice();
    q::QueryBuilder         b;
    auto                    schema      = b.use(database_tables);
    auto                    artist      = schema.table<Artist>();
    auto                    item        = schema.table<Item>();
    auto                    library_ids = b.param_group<i64>("libraries"_str, libraries.len());
    ArtistFilterExpressions filters(b);
    auto                    condition = item.col<&Item::library_id>().in(library_ids);
    auto                    extra     = filters.build(schema, artist, request.filters);
    if (extra) condition = condition && *extra;
    auto select =
        b.select(artist.col<&Artist::id>())
            .distinct()
            .from(artist.source())
            .join(item.source(), b.on(schema.relation(db_relations::artist_item), artist, item));
    if (album_artists) {
        auto relation = schema.table<RelAlbumArtist>();
        select        = select.join(
            relation.source(),
            b.on(schema.relation(db_relations::rel_album_artist_artist), relation, artist));
    } else {
        auto relation = schema.table<RelSongArtist>();
        select        = select.join(
            relation.source(),
            b.on(schema.relation(db_relations::rel_song_artist_artist), relation, artist));
    }
    auto order = artist_order(artist, request.sort);
    select     = select.where(condition).order_by(request.ascending ? order.asc() : order.desc(),
                                                  artist.col<&Artist::id>().asc());
    auto query = rstd_try(
        q::try_compile_page(select, i64(request.page.size.to_primitive()), request.page.offset));
    auto stmt = rstd_try(liorm::sqlito::prepare_page(db, rstd::move(query)));
    rstd_try(stmt.bind_group("libraries"_str, libraries));
    rstd_try(filters.bind(stmt));
    auto rows =
        rstd_try(stmt.fetch(QueryIdProjection { "artist_query returned a null ID or count"_str }));
    ArtistQueryPage result;
    result.total = rows.total;
    result.ids   = rstd::move(rows.rows);
    return Ok(rstd::move(result));
}

template auto artist_query_page(sqlito::Connection&, const ArtistQuery&, bool)
    -> QueryResult<ArtistQueryPage>;
template auto artist_query_page(sqlito::Transaction&, const ArtistQuery&, bool)
    -> QueryResult<ArtistQueryPage>;
} // namespace qcm::backend
