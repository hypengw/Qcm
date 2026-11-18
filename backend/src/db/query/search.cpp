module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.query.search;
import :db.schema;
import :fts;
import :model.album;
import :model.artist;
import :model.item;
import :model.song;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend::search_detail
{
namespace q = liorm::query;
auto search_fts_name(SearchKind kind) -> ref<str> {
    switch (kind) {
    case SearchKind::Album: return "album_fts"_str;
    case SearchKind::Artist: return "artist_fts"_str;
    case SearchKind::Song: return "song_fts"_str;
    }
}
auto search_statement(SearchKind kind, usize library_count, Page page)
    -> Result<q::PageQuery, q::Error> {
    q::QueryBuilder b;
    auto            schema = b.use(database_tables);
    auto            item   = schema.table<Item>();
    q::BoundTable   entity;
    q::Expr         id;
    q::Expr         item_condition;
    switch (kind) {
    case SearchKind::Album: {
        auto album     = schema.table<Album>();
        entity         = album;
        id             = album.col<&Album::id>();
        item_condition = b.on(schema.relation(db_relations::album_item), album, item);
        break;
    }
    case SearchKind::Artist: {
        auto artist    = schema.table<Artist>();
        entity         = artist;
        id             = artist.col<&Artist::id>();
        item_condition = b.on(schema.relation(db_relations::artist_item), artist, item);
        break;
    }
    case SearchKind::Song: {
        auto song      = schema.table<Song>();
        entity         = song;
        id             = song.col<&Song::id>();
        item_condition = b.on(schema.relation(db_relations::song_item), song, item);
        break;
    }
    }
    auto fts_name = search_fts_name(kind);
    auto fts      = b.use(qcm_fts_tables).table(fts_name);
    auto query    = b.select(id)
                        .from(entity.source())
                        .join(item.source(), item_condition)
                        .join(fts.source(), id == fts.col("rowid"_str))
                        .where(fts.col(fts_name).match(b.param<q::TextParameter>("query"_str)) &&
                               item.col<&Item::library_id>().in(
                                   b.param_group<i64>("libraries"_str, library_count)))
                        .order_by(id.asc());
    return q::try_compile_page(query, i64(page.size.to_primitive()), page.offset);
}
} // namespace qcm::backend::search_detail

namespace qcm::backend
{
template<class Database>
auto search_page(Database& db, SearchQuery input) -> QueryResult<IdQueryPage> {
    using namespace search_detail;
    IdQueryPage                     result;
    auto                            cancel = db.cancellation();
    liunicode::NormalizationControl control {
        &cancel,
        [](const void* value) noexcept -> bool {
            const auto& token = *static_cast<const Option<sqlito::Cancellation>*>(value);
            return token.is_some() && token->cancelled();
        }
    };
    auto normalized = qcm_fts_query(input.text.as_str().as_bytes(), control);
    if (normalized.is_err())
        return Err(QueryError(sqlito::Error::from(
            normalized.unwrap_err() == liunicode::Error::Cancelled ? sqlito::ErrorKind::Cancelled
                                                                   : sqlito::ErrorKind::Input,
            0,
            String::make("qcm query failed"_str))));
    auto text = String::make("name:"_str);
    text.push_str(rstd::move(normalized).unwrap().as_str());
    auto query = rstd_try(search_statement(input.kind, input.libraries.len(), input.page));
    auto list  = rstd_try(liorm::sqlito::prepare_page(db, rstd::move(query)));
    rstd_try(list.bind("query"_str, text));
    rstd_try(list.bind_group("libraries"_str, input.libraries.as_slice()));
    auto rows    = rstd_try(list.fetch(liorm::TupleProjection<i64> {}));
    result.total = rows.total;
    for (auto& row : rows.rows) result.ids.push(rstd::move(rstd::get<0>(row)));
    return Ok(rstd::move(result));
}

template auto search_page(sqlito::Connection&, SearchQuery) -> QueryResult<IdQueryPage>;
template auto search_page(sqlito::Transaction&, SearchQuery) -> QueryResult<IdQueryPage>;
} // namespace qcm::backend
