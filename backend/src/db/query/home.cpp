module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.query.home;
import :model.album;
import :model.artist;
import :model.mix;
import :model.song;
import :db.error.query;
import :model.remote_mix;
import :db.schema;
import liorm;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace q = liorm::query;

namespace qcm::backend
{
namespace
{
template<class Model>
constexpr q::ProjectionLayout<q::ModelGroup<Model>> home_item_projection;

template<class Model>
constexpr auto home_item_query = q::compile([](q::QueryBuilder& b) {
    auto item = b.use(database_tables).template table<Model>();
    return b.select(q::project(home_item_projection<Model>, item).as_slice())
        .from(item.source())
        .where(item.template col<&Model::id>() == b.param<i64>("id"_str))
        .limit(b.integer(1));
});

template<class Model>
auto home_item(sqlito::Transaction& tx, i64 id) -> QueryResult<Option<Model>> {
    return Ok(rstd_try(liorm::sqlito::fetch_model<home_item_query<Model>>(
        tx,
        home_item_projection<Model>.template group<0>(),
        liorm::parameters(liorm::param<"id">(id)))));
}

inline constexpr auto home_daily_query     = q::compile([](q::QueryBuilder& b) {
    auto schema = b.use(database_tables);
    auto mix    = schema.table<RemoteMix>();
    auto item   = schema.table<Item>();
    return b.select(mix.col<&RemoteMix::id>())
        .from(mix.source())
        .join(item.source(), b.on(schema.relation(db_relations::remote_mix_item), mix, item))
        .where((item.col<&Item::provider_id>() == b.param<i64>("provider"_str)) &&
               (item.col<&Item::type>() == b.param<i64>("item_type"_str)) &&
               (mix.col<&RemoteMix::mix_type>() == b.param<q::TextParameter>("mix_type"_str)))
        .order_by(mix.col<&RemoteMix::id>().asc());
});
inline constexpr auto home_local_mix_query = q::compile([](q::QueryBuilder& b) {
    auto mix = b.use(database_tables).table<Mix>();
    return b.select(q::project(mix_projection, mix).as_slice())
        .from(mix.source())
        .where(mix.col<&Mix::remote_id>() == b.param<i64>("remote_id"_str))
        .limit(b.integer(1));
});
} // namespace

auto home_daily_ids(sqlito::Connection& db, i64 provider) -> QueryResult<Vec<i64>> {
    auto     rows = rstd_try(liorm::sqlito::fetch_all<home_daily_query>(
        db,
        liorm::TupleProjection<Option<i64>> {},
        liorm::parameters(liorm::param<"provider">(provider),
                          liorm::param<"item_type">(i64(54)),
                          liorm::param<"mix_type">("daily"_str))));
    Vec<i64> result;
    for (auto& row : rows) {
        auto id = rstd::get<0>(row);
        if (id.is_some()) result.push(i64(*id));
    }
    return Ok(rstd::move(result));
}
auto home_song(sqlito::Transaction& tx, i64 id) -> QueryResult<Option<Song>> {
    return home_item<Song>(tx, id);
}
auto home_album(sqlito::Transaction& tx, i64 id) -> QueryResult<Option<Album>> {
    return home_item<Album>(tx, id);
}
auto home_artist(sqlito::Transaction& tx, i64 id) -> QueryResult<Option<Artist>> {
    return home_item<Artist>(tx, id);
}
auto home_local_mix(sqlito::Transaction& tx, i64 remote_id) -> QueryResult<Option<Mix>> {
    return Ok(rstd_try(liorm::sqlito::fetch_model<home_local_mix_query>(
        tx, mix_projection.group<0>(), liorm::parameters(liorm::param<"remote_id">(remote_id)))));
}
} // namespace qcm::backend
