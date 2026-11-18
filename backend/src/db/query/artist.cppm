module;
#include <rstd/enum.hpp>
export module qcm.backend:db.query.artist;
export import :db.query.input;
export import :db.error.query;
export import sqlito;
import liorm;
import :db.query.filter_expressions;
import :db.schema;
import :model.artist;
import :model.album;
import :model.rel_album_artist;
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
    -> QueryResult<ArtistQueryPage>;

extern template auto artist_query_page(sqlito::Connection&, const ArtistQuery&, bool)
    -> QueryResult<ArtistQueryPage>;
extern template auto artist_query_page(sqlito::Transaction&, const ArtistQuery&, bool)
    -> QueryResult<ArtistQueryPage>;
} // namespace qcm::backend
