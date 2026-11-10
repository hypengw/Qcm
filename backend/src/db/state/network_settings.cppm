module;
#include <rstd/macro.hpp>
export module qcm.backend:db.state.network_settings;
export import :error;
export import :network;
export import sqlito;
import :db.provider.schema;
import liorm.sqlito;
using rstd::sync::Mutex;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace network_settings_query = liorm::query;

namespace qcm::backend::network_settings_db
{
struct [[pmacro::derive("liorm::table"), pmacro::helper("liorm::table", name("network_settings"),
                                                        primary("network_settings_pk"))]] Revision {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 revision;
};
struct [[pmacro::derive("liorm::table"), pmacro::helper("liorm::table", name("network_proxy"),
                                                        primary("network_proxy_pk"))]] Proxy {
    [[pmacro::helper("liorm::table", storage(text), primary)]]
    String proxy_id;
    [[pmacro::helper("liorm::table", storage(text))]]
    String name;
    [[pmacro::helper("liorm::table", storage(text))]]
    String url;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 position;
};
struct [[pmacro::derive("liorm::table"), pmacro::helper("liorm::table", name("provider_proxy"),
                                                        primary("provider_proxy_pk"))]] Route {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 provider_id;
    [[pmacro::helper("liorm::table", storage(text))]]
    String proxy_id;
};
inline constexpr auto tables = liorm::schema::define([](liorm::schema::SchemaBuilder& b) {
    liorm::schema::register_table<Revision>(b);
    liorm::schema::register_table<Proxy>(b);
    liorm::schema::register_table<Route>(b);
});
Mutex<empty>          edits { empty {} };
namespace q = network_settings_query;
inline constexpr liorm::RecordProjection<network::ProxyEntry, &network::ProxyEntry::id,
                                         &network::ProxyEntry::name, &network::ProxyEntry::url>
                                             proxy_projection;
inline constexpr liorm::TupleProjection<i64> route_projection;
inline constexpr auto                        revision_query = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(tables).table<Revision>();
    return b.select(t.col<&Revision::revision>())
        .from(t.source())
        .where(t.col<&Revision::id>() == b.integer(1));
});
inline constexpr auto                        proxies_query  = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(tables).table<Proxy>();
    return proxy_projection
        .select(b, t.col<&Proxy::proxy_id>(), t.col<&Proxy::name>(), t.col<&Proxy::url>())
        .from(t.source())
        .order_by(t.col<&Proxy::position>().asc());
});
inline constexpr auto                        routes_query   = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(tables).table<Route>();
    return route_projection.select(b, t.col<&Route::provider_id>())
        .from(t.source())
        .where(t.col<&Route::proxy_id>() == b.param<q::TextParameter>("proxy"_str))
        .order_by(t.col<&Route::provider_id>().asc());
});
inline constexpr auto                        provider_query = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(provider_tables).table<ProviderRecord>();
    return b.select(t.col<&ProviderRecord::provider_id>())
        .from(t.source())
        .where(t.col<&ProviderRecord::provider_id>() == b.param<i64>("provider"_str));
});
inline constexpr auto                        advance_query  = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(tables).table<Revision>();
    return b.update(t.source())
        .set(t.col<&Revision::revision>().set(t.col<&Revision::revision>() + b.integer(1)))
        .where((t.col<&Revision::id>() == b.integer(1)) &&
               (t.col<&Revision::revision>() == b.param<i64>("revision"_str)));
});
inline constexpr auto                        clear_query    = q::compile([](q::QueryBuilder& b) {
    return b.delete_from(b.use(tables).table<Proxy>().source()).allow_all();
});
inline constexpr auto insert_proxy_query                    = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(tables).table<Proxy>();
    return b
        .insert_into(t.source(),
                     t.col<&Proxy::proxy_id>(),
                     t.col<&Proxy::name>(),
                     t.col<&Proxy::url>(),
                     t.col<&Proxy::position>())
        .values(b.param<q::TextParameter>("id"_str),
                b.param<q::TextParameter>("name"_str),
                b.param<q::TextParameter>("url"_str),
                b.param<i64>("position"_str));
});
inline constexpr auto insert_route_query                    = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(tables).table<Route>();
    return b.insert_into(t.source(), t.col<&Route::provider_id>(), t.col<&Route::proxy_id>())
        .values(b.param<i64>("provider"_str), b.param<q::TextParameter>("proxy"_str));
});
auto                  load(sqlito::Transaction& db) -> ProcessResult<network::NetworkSettings> {
    network::NetworkSettings settings;
    auto                     revision = rstd_try(liorm::sqlito::prepare(db, revision_query));
    auto                     row      = rstd_try(liorm::sqlito::optional_scalar<i64>(
        revision, usize(), liorm::sqlito::RowConsumption::First));
    if (row.is_none()) return Err(ProcessError::internal("Network settings are missing"_str));
    auto value = *row;
    if (value < i64()) return Err(ProcessError::internal("Invalid network settings revision"_str));
    settings.revision = u64(value.to_primitive());
    auto proxies      = rstd_try(liorm::sqlito::prepare(db, proxies_query));
    while (rstd_try(proxies.step()) == sqlito::Step::Row) {
        auto reader = proxies.reader();
        auto entry  = rstd_try(proxy_projection.decode(reader));
        auto routes = rstd_try(liorm::sqlito::prepare(db, routes_query));
        rstd_try(routes.bind("proxy"_str, entry.id));
        auto rows = rstd_try(liorm::sqlito::all_rows(routes, route_projection));
        for (auto& row : rows) entry.provider_ids.push(rstd::move(rstd::get<0>(row)));
        settings.proxies.push(rstd::move(entry));
    }
    return Ok(rstd::move(settings));
}
auto save(sqlito::Transaction& db, const network::NetworkSettings& settings)
    -> ProcessResult<empty> {
    if (settings.revision >= u64(i64::MAX.to_primitive()))
        return Err(ProcessError::internal("Invalid network settings revision"_str));
    for (const auto& proxy : settings.proxies)
        for (auto provider : proxy.provider_ids) {
            auto check = rstd_try(liorm::sqlito::prepare(db, provider_query));
            rstd_try(check.bind("provider"_str, provider));
            if (rstd_try(check.step()) == sqlito::Step::Done)
                return Err(ProcessError::internal(
                    "A selected provider no longer exists; reload before saving"_str));
        }
    auto update = rstd_try(liorm::sqlito::prepare(db, advance_query));
    rstd_try(update.bind("revision"_str, i64(settings.revision.to_primitive())));
    rstd_try(liorm::sqlito::discard(update));
    if (rstd_try(db.last_changes()) != u64(1))
        return Err(ProcessError::internal("Network settings changed; reload before saving"_str));
    auto clear = rstd_try(liorm::sqlito::prepare(db, clear_query));
    rstd_try(liorm::sqlito::discard(clear));
    for (usize position {}; position < settings.proxies.len(); ++position) {
        const auto& proxy  = settings.proxies[position];
        auto        insert = rstd_try(liorm::sqlito::prepare(db, insert_proxy_query));
        rstd_try(insert.bind("id"_str, proxy.id));
        rstd_try(insert.bind("name"_str, proxy.name));
        rstd_try(insert.bind("url"_str, proxy.url));
        rstd_try(insert.bind("position"_str, i64(position.to_primitive())));
        rstd_try(liorm::sqlito::discard(insert));
        for (auto provider : proxy.provider_ids) {
            auto link = rstd_try(liorm::sqlito::prepare(db, insert_route_query));
            rstd_try(link.bind("provider"_str, provider));
            rstd_try(link.bind("proxy"_str, proxy.id));
            rstd_try(liorm::sqlito::discard(link));
        }
    }
    return Ok(empty {});
}
} // namespace qcm::backend::network_settings_db

export namespace qcm::backend
{
auto network_settings_snapshot(sqlito::Connection& db) -> ProcessResult<network::NetworkSettings> {
    auto edit     = network_settings_db::edits.lock().unwrap();
    auto tx       = rstd_try(db.begin());
    auto compiled = rstd_try(network::compile(rstd_try(network_settings_db::load(tx))),
                             ProcessError::internal(rstd::move(_e)));
    rstd_try(tx.commit());
    return Ok(rstd::move(compiled.settings));
}
auto restore_network_policy(sqlito::Connection& db) -> ProcessResult<empty> {
    auto edit     = network_settings_db::edits.lock().unwrap();
    auto tx       = rstd_try(db.begin());
    auto compiled = rstd_try(network::compile(rstd_try(network_settings_db::load(tx))),
                             ProcessError::internal(rstd::move(_e)));
    rstd_try(tx.commit());
    network::publish(rstd::move(compiled));
    return Ok(empty {});
}
auto store_network_settings(sqlito::Connection& db, network::CompiledSettings compiled)
    -> ProcessResult<empty> {
    auto edit = network_settings_db::edits.lock().unwrap();
    auto tx   = rstd_try(db.begin());
    rstd_try(network_settings_db::save(tx, compiled.settings));
    rstd_try(tx.commit());
    network::publish(rstd::move(compiled));
    return Ok(empty {});
}
} // namespace qcm::backend
