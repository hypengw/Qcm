module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.query.subtitle;
import sqlito;
import liorm.sqlito;
import :db.schema;
import :db.error.query;
import :model.item;
import :model.song;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace subtitle_query = liorm::query;

namespace qcm::backend
{
struct SubtitleTargetFields {
    Option<i64>    provider_id;
    Option<String> native_id;
};
inline constexpr liorm::RecordProjection<SubtitleTargetFields, &SubtitleTargetFields::provider_id,
                                         &SubtitleTargetFields::native_id>
                      subtitle_target_projection;
inline constexpr auto subtitle_item_query =
    subtitle_query::compile([](subtitle_query::QueryBuilder& b) {
        auto schema = b.use(database_tables);
        auto song   = schema.table<Song>();
        auto item   = schema.table<Item>();
        return subtitle_target_projection
            .select(b, item.col<&Item::provider_id>(), item.col<&Item::native_id>())
            .from(song.source())
            .join(item.source(), b.on(schema.relation(db_relations::song_item), song, item))
            .where(song.col<&Song::id>() == b.param<i64>("id"_str));
    });
} // namespace qcm::backend

namespace qcm::backend
{
auto subtitle_target(sqlito::Connection& db, i64 id) -> QueryResult<Option<SubtitleTarget>> {
    auto fields = rstd_try(
        liorm::sqlito::fetch_one<subtitle_item_query>(db,
                                                      subtitle_target_projection,
                                                      liorm::parameters(liorm::param<"id">(id)),
                                                      liorm::sqlito::RowConsumption::First));
    if (fields.is_none()) return Ok(None());
    if (fields->provider_id.is_none() || fields->native_id.is_none())
        return Err(sqlito::Error::from(
            sqlito::ErrorKind::Type, 0, String::make("invalid subtitle target"_str)));
    return Ok(Some(SubtitleTarget { *fields->provider_id, rstd::move(*fields->native_id) }));
}
} // namespace qcm::backend
