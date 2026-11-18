module;
#include <rstd/enum.hpp>
export module qcm.backend:db.query.song;
export import :db.query.input;
export import :db.error.query;
export import sqlito;
import liorm;
import :db.query.filter_expressions;
import :db.schema;
import :model.song;
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
auto filtered_song_ids(Database& db, const FilteredSongQuery& request) -> QueryResult<Vec<i64>>;

extern template auto filtered_song_ids(sqlito::Connection&, const FilteredSongQuery&)
    -> QueryResult<Vec<i64>>;
extern template auto filtered_song_ids(sqlito::Transaction&, const FilteredSongQuery&)
    -> QueryResult<Vec<i64>>;
} // namespace qcm::backend
