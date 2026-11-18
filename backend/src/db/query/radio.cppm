export module qcm.backend:db.query.radio;
export import sqlito;
import liorm;
import :db.schema;
import :db.error.query;
import :model.song;
import :model.item;
import :model.remote_mix;
export import rstd;
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
auto radio_queue_row(sqlito::Connection& db, i64 id) -> QueryResult<Option<RadioQueueRow>>;
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
auto radio_queue_rows(sqlito::Connection& db, slice<i64> providers)
    -> QueryResult<Vec<RadioQueueRow>>;
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
auto radio_ordered_songs(sqlito::Connection& db, i64 provider, slice<i64> ids)
    -> QueryResult<Vec<i64>>;
} // namespace qcm::backend
