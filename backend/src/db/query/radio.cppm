module;
#include <rstd/macro.hpp>
export module qcm.backend:db.query.radio;
export import sqlito;
import liorm;
import liorm.sqlito;
import :db.schema;
import :db.error.query;
import :model.song;
import :model.item;
import :model.remote_mix;
export import rstd;
using rstd::collections::HashSet;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace q = liorm::query;

namespace qcm::backend
{
struct RadioQueueFields {
    Option<i64>    id;
    Option<String> name;
    Option<String> description;
    Option<String> native_id;
    Option<i64>    provider_id;
};
inline constexpr liorm::RecordProjection<
    RadioQueueFields, &RadioQueueFields::name, &RadioQueueFields::description,
    &RadioQueueFields::native_id, &RadioQueueFields::provider_id>
    radio_queue_projection;
inline constexpr liorm::RecordProjection<
    RadioQueueFields, &RadioQueueFields::id, &RadioQueueFields::name,
    &RadioQueueFields::description, &RadioQueueFields::native_id, &RadioQueueFields::provider_id>
    radio_queues_projection;
} // namespace qcm::backend

export namespace qcm::backend
{
struct RadioQueueRow {
    i64    id;
    i64    provider_id;
    String name;
    String description;
    String native_id;
};
inline constexpr auto radio_queue_find_query = q::compile([](q::QueryBuilder& b) {
    auto schema = b.use(database_tables);
    auto mix    = schema.table<RemoteMix>();
    auto item   = schema.table<Item>();
    return radio_queue_projection
        .select(b,
                mix.col<&RemoteMix::name>(),
                mix.col<&RemoteMix::description>(),
                item.col<&Item::native_id>(),
                item.col<&Item::provider_id>())
        .from(mix.source())
        .join(item.source(), b.on(schema.relation(db_relations::remote_mix_item), mix, item))
        .where((mix.col<&RemoteMix::id>() == b.param<i64>("id"_str)) &&
               (mix.col<&RemoteMix::mix_type>() == b.text_value("queue"_str)) &&
               (item.col<&Item::type>() == b.param<i64>("type"_str)))
        .limit(b.integer(1));
});
auto radio_queue_row(sqlito::Connection& db, i64 id) -> QueryResult<Option<RadioQueueRow>> {
    auto row = rstd_try(liorm::sqlito::fetch_one<radio_queue_find_query>(
        db,
        radio_queue_projection,
        liorm::parameters(liorm::param<"id">(id),
                          liorm::param<"type">(i64(static_cast<int>(ItemType::Radio)))),
        liorm::sqlito::RowConsumption::First));
    if (row.is_none()) return Ok(None<RadioQueueRow>());
    return Ok(Some(RadioQueueRow { id,
                                   row->provider_id.unwrap_or(i64()),
                                   row->name.unwrap_or(String::make()),
                                   row->description.unwrap_or(String::make()),
                                   row->native_id.unwrap_or(String::make()) }));
}
inline constexpr auto radio_queues_query = q::compile([](q::QueryBuilder& b) {
    auto schema = b.use(database_tables);
    auto mix    = schema.table<RemoteMix>();
    auto item   = schema.table<Item>();
    return radio_queues_projection
        .select(b,
                mix.col<&RemoteMix::id>(),
                mix.col<&RemoteMix::name>(),
                mix.col<&RemoteMix::description>(),
                item.col<&Item::native_id>(),
                item.col<&Item::provider_id>())
        .from(mix.source())
        .join(item.source(), b.on(schema.relation(db_relations::remote_mix_item), mix, item))
        .where((mix.col<&RemoteMix::mix_type>() == b.param<q::TextParameter>("type"_str)) &&
               (item.col<&Item::type>() == b.param<i64>("item_type"_str)))
        .order_by(mix.col<&RemoteMix::id>().asc());
});
auto                  radio_queue_rows(sqlito::Connection& db, slice<i64> providers)
    -> QueryResult<Vec<RadioQueueRow>> {
    auto               values = rstd_try(liorm::sqlito::fetch_all<radio_queues_query>(
        db,
        radio_queues_projection,
        liorm::parameters(liorm::param<"type">("queue"_str), liorm::param<"item_type">(i64(55)))));
    Vec<RadioQueueRow> rows;
    for (auto& fields : values) {
        auto provider_id = fields.provider_id.unwrap_or(i64());
        bool included    = false;
        for (auto wanted : providers)
            if (wanted == provider_id) {
                included = true;
                break;
            }
        if (included)
            rows.push(RadioQueueRow { fields.id.unwrap_or(i64()),
                                      provider_id,
                                      fields.name.unwrap_or(String::make()),
                                      fields.description.unwrap_or(String::make()),
                                      fields.native_id.unwrap_or(String::make()) });
    }
    return Ok(rstd::move(rows));
}
inline constexpr auto radio_song_query = q::compile([](q::QueryBuilder& b) {
    auto schema = b.use(database_tables);
    auto song   = schema.table<Song>();
    auto item   = schema.table<Item>();
    return b.select(b.integer(1))
        .from(song.source())
        .join(item.source(), b.on(schema.relation(db_relations::song_item), song, item))
        .where((song.col<&Song::id>() == b.param<i64>("id"_str)) &&
               (item.col<&Item::provider_id>() == b.param<i64>("provider"_str)))
        .limit(b.integer(1));
});
auto                  radio_ordered_songs(sqlito::Connection& db, i64 provider, slice<i64> ids)
    -> QueryResult<Vec<i64>> {
    auto         query = rstd_try(liorm::sqlito::prepare<radio_song_query>(
        db, liorm::parameters(liorm::param<"id">(i64()), liorm::param<"provider">(provider))));
    HashSet<i64> seen;
    Vec<i64>     result;
    for (auto id : ids) {
        if (! seen.insert(i64(id))) continue;
        (void)rstd_try(query.reset());
        rstd_try(query.bind(
            liorm::parameters(liorm::param<"id">(id), liorm::param<"provider">(provider))));
        if (rstd_try(liorm::sqlito::row_exists(query))) result.push(i64(id));
    }
    return Ok(rstd::move(result));
}
} // namespace qcm::backend
