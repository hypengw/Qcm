module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:db.query.song;
export import :db.query.input;
export import :db.error.query;
export import sqlito;
import liorm;
import liorm.sqlito;
import :db.query.filter_expressions;
import :db.query.album;
import :db.schema;
import :model.song;
import :model.item;
import :model.rel_mix_song;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace q = liorm::query;

export namespace qcm::backend
{
class SongFilterExpressions : public FilterExpressions {
public:
    using FilterExpressions::FilterExpressions;

    template<class SongTable>
    auto build(q::BoundSchema schema, const SongTable& song,
               const QueryFilters<SongFilter>& filters) -> Option<q::Expr> {
        for (const auto& filter : filters.values) {
            Option<q::Expr> expr;
            RSTD_MATCH(filter.value) {
                RSTD_CASE(AlbumId, id) {
                    expr = Some(song.template col<&Song::album_id>() == number(id));
                }
                RSTD_CASE(MixId, id) {
                    auto relation = schema.template table<RelMixSong>();
                    expr          = Some(builder_.exists(
                        builder_.select(builder_.integer(1))
                            .from(relation.source())
                            .where(builder_.on(schema.relation(db_relations::rel_mix_song_song),
                                               relation,
                                               song) &&
                                   (relation.template col<&RelMixSong::mix_id>() == number(id)))));
                }
            }
            add(filter.group.to_primitive(), expr);
        }
        return grouped(filters.logics);
    }
};

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
} // namespace qcm::backend
