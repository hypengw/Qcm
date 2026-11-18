module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:db.query.artist;
export import :db.query.input;
export import :db.error.query;
export import sqlito;
import liorm;
import liorm.sqlito;
import :db.query.filter_expressions;
import :db.schema;
import :model.item;
import :model.artist;
import :model.album;
import :model.rel_song_artist;
import :model.rel_album_artist;
import :pagination;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace q = liorm::query;

export namespace qcm::backend
{
auto artist_exists(sqlito::Connection& db, i64 id) -> QueryResult<bool>;
auto artist_exists(sqlito::Transaction& db, i64 id) -> QueryResult<bool>;

class ArtistFilterExpressions : public FilterExpressions {
public:
    using FilterExpressions::FilterExpressions;

    template<class ArtistTable>
    auto build(q::BoundSchema schema, const ArtistTable& artist,
               const QueryFilters<ArtistFilter>& filters) -> Option<q::Expr> {
        for (const auto& filter : filters.values) {
            Option<q::Expr> expr;
            RSTD_MATCH(filter.value) {
                RSTD_CASE(Name, f) {
                    expr =
                        string(artist.template col<&Artist::name>(), f.value.as_str(), f.condition);
                }
                RSTD_CASE(AlbumName, f) {
                    auto album    = schema.template table<Album>();
                    auto relation = schema.template table<RelAlbumArtist>();
                    auto title =
                        string(album.template col<&Album::name>(), f.value.as_str(), f.condition);
                    if (title)
                        expr = Some(builder_.exists(
                            builder_.select(builder_.integer(1))
                                .from(album.source())
                                .join(relation.source(),
                                      builder_.on(
                                          schema.relation(db_relations::rel_album_artist_album),
                                          relation,
                                          album))
                                .where(builder_.on(
                                           schema.relation(db_relations::rel_album_artist_artist),
                                           relation,
                                           artist) &&
                                       *title)));
                }
                RSTD_CASE(AddedAt, f) {
                    expr = date(artist.template col<&Artist::added_at>(), f.value, f.condition);
                }
            }
            add(filter.group.to_primitive(), expr);
        }
        return grouped(filters.logics);
    }
};
struct ArtistQueryPage {
    Vec<i64> ids;
    i64      total {};
};
template<class ArtistTable>
auto artist_order(const ArtistTable& artist, ArtistSort sort) -> q::Expr {
    switch (sort) {
    case ArtistSort::SortName: return artist.template col<&Artist::sort_name>();
    case ArtistSort::AlbumCount: return artist.template col<&Artist::album_count>();
    case ArtistSort::MusicCount: return artist.template col<&Artist::music_count>();
    case ArtistSort::Name: return artist.template col<&Artist::name>();
    }
    return artist.template col<&Artist::name>();
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
} // namespace qcm::backend
