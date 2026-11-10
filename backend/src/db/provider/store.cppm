module;
#include <rstd/macro.hpp>
export module qcm.backend:db.provider.store;
export import :error;
export import :db.provider.schema;
export import :db.sync.run;
export import sqlito;
import :db.timestamp;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace provider_store_query = liorm::query;

export namespace qcm::backend
{
struct ProviderSessionWrite {
    String base_url;
    String auth_method;
    String custom;
    String cookie;
    String session_key;
};
struct ProviderCreate {
    String               name;
    String               type_name;
    ProviderSessionWrite session;
};
struct ProviderIdentity {
    String type_name;
    String session_key;
};
struct ProviderStoredSettings {
    String name;
    String config;
    i64    revision;
};
struct ProviderSettingsWrite {
    String config;
    u64    revision;
};
struct ProviderUpdate {
    i64                          provider_id;
    String                       name;
    String                       session_key;
    Option<ProviderSessionWrite> replacement;
    ProviderSettingsWrite        settings;
};
} // namespace qcm::backend

namespace qcm::backend::provider_store
{
namespace q = provider_store_query;
struct IdentityFields {
    Option<String> type_name;
    Option<String> session_key;
};
struct SettingsFields {
    Option<String> name;
    Option<String> config;
    Option<i64>    revision;
};
inline constexpr liorm::RecordProjection<IdentityFields, &IdentityFields::type_name,
                                         &IdentityFields::session_key>
    identity_projection;
inline constexpr liorm::RecordProjection<SettingsFields, &SettingsFields::name,
                                         &SettingsFields::config, &SettingsFields::revision>
                      settings_projection;
inline constexpr auto create_query       = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(provider_tables).table<ProviderRecord>();
    return b
        .insert_into(t.source(),
                     t.col<&ProviderRecord::name>(),
                     t.col<&ProviderRecord::type_name>(),
                     t.col<&ProviderRecord::base_url>(),
                     t.col<&ProviderRecord::auth_method>(),
                     t.col<&ProviderRecord::cookie>(),
                     t.col<&ProviderRecord::custom>(),
                     t.col<&ProviderRecord::session_key>(),
                     t.col<&ProviderRecord::edit_time>())
        .values(b.param<q::TextParameter>("name"_str),
                b.param<q::TextParameter>("type"_str),
                b.param<q::TextParameter>("base"_str),
                b.param<q::Nullable<q::TextParameter>>("auth"_str),
                b.param<q::TextParameter>("cookie"_str),
                b.param<q::TextParameter>("custom"_str),
                b.param<q::TextParameter>("key"_str),
                b.param<i64>("now"_str))
        .returning(t.col<&ProviderRecord::provider_id>());
});
inline constexpr auto identity_query     = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(provider_tables).table<ProviderRecord>();
    return identity_projection
        .select(b, t.col<&ProviderRecord::type_name>(), t.col<&ProviderRecord::session_key>())
        .from(t.source())
        .where(t.col<&ProviderRecord::provider_id>() == b.param<i64>("id"_str));
});
inline constexpr auto revision_query     = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(provider_tables).table<ProviderSettingsRecord>();
    return b.select(t.col<&ProviderSettingsRecord::revision>())
        .from(t.source())
        .where(t.col<&ProviderSettingsRecord::provider_id>() == b.param<i64>("id"_str));
});
inline constexpr auto settings_query     = q::compile([](q::QueryBuilder& b) {
    auto schema = b.use(provider_tables);
    auto t      = schema.table<ProviderRecord>();
    auto s      = schema.table<ProviderSettingsRecord>();
    return settings_projection
        .select(b,
                t.col<&ProviderRecord::name>(),
                s.col<&ProviderSettingsRecord::config>(),
                s.col<&ProviderSettingsRecord::revision>())
        .from(t.source())
        .left_join(s.source(),
                   t.col<&ProviderRecord::provider_id>() ==
                       s.col<&ProviderSettingsRecord::provider_id>())
        .where(t.col<&ProviderRecord::provider_id>() == b.param<i64>("id"_str));
});
inline constexpr auto settings_upsert    = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(provider_tables).table<ProviderSettingsRecord>();
    return b
        .insert_into(t.source(),
                     t.col<&ProviderSettingsRecord::provider_id>(),
                     t.col<&ProviderSettingsRecord::revision>(),
                     t.col<&ProviderSettingsRecord::config>())
        .values(b.param<i64>("id"_str),
                b.param<i64>("revision"_str),
                b.param<q::TextParameter>("config"_str))
        .on_conflict_do_update(
            array<q::Expr, 1> { t.col<&ProviderSettingsRecord::provider_id>() }.as_slice(),
            array<q::Expr, 2> { t.col<&ProviderSettingsRecord::revision>().set(
                                    b.excluded(t.col<&ProviderSettingsRecord::revision>())),
                                t.col<&ProviderSettingsRecord::config>().set(
                                    b.excluded(t.col<&ProviderSettingsRecord::config>())) }
                .as_slice());
});
inline constexpr auto rename_query       = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(provider_tables).table<ProviderRecord>();
    return b.update(t.source())
        .set(t.col<&ProviderRecord::name>().set(b.param<q::TextParameter>("name"_str)))
        .where(
            (t.col<&ProviderRecord::provider_id>() == b.param<i64>("id"_str)) &&
            (t.col<&ProviderRecord::session_key>() == b.param<q::TextParameter>("previous"_str)));
});
inline constexpr auto replace_query      = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(provider_tables).table<ProviderRecord>();
    return b.update(t.source())
        .set(t.col<&ProviderRecord::name>().set(b.param<q::TextParameter>("name"_str)),
             t.col<&ProviderRecord::base_url>().set(b.param<q::TextParameter>("base"_str)),
             t.col<&ProviderRecord::auth_method>().set(
                 b.param<q::Nullable<q::TextParameter>>("auth"_str)),
             t.col<&ProviderRecord::custom>().set(b.param<q::TextParameter>("custom"_str)),
             t.col<&ProviderRecord::cookie>().set(b.param<q::TextParameter>("cookie"_str)),
             t.col<&ProviderRecord::session_key>().set(b.param<q::TextParameter>("key"_str)),
             t.col<&ProviderRecord::edit_time>().set(b.param<i64>("now"_str)))
        .where(
            (t.col<&ProviderRecord::provider_id>() == b.param<i64>("id"_str)) &&
            (t.col<&ProviderRecord::session_key>() == b.param<q::TextParameter>("previous"_str)));
});
inline constexpr auto session_save_query = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(provider_tables).table<ProviderRecord>();
    return b.update(t.source())
        .set(t.col<&ProviderRecord::custom>().set(b.param<q::TextParameter>("custom"_str)),
             t.col<&ProviderRecord::cookie>().set(b.param<q::TextParameter>("cookie"_str)))
        .where((t.col<&ProviderRecord::provider_id>() == b.param<i64>("id"_str)) &&
               (t.col<&ProviderRecord::session_key>() == b.param<q::TextParameter>("key"_str)));
});
inline constexpr auto delete_query       = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(provider_tables).table<ProviderRecord>();
    return b.delete_from(t.source())
        .where(t.col<&ProviderRecord::provider_id>() == b.param<i64>("id"_str));
});
template<class Statement>
auto bind_session(Statement& stmt, const ProviderSessionWrite& session) -> ProcessResult<empty> {
    rstd_try(stmt.bind("base"_str, session.base_url));
    auto auth =
        session.auth_method.is_empty() ? Option<String>(None()) : Some(session.auth_method.clone());
    rstd_try(stmt.bind("auth"_str, auth));
    rstd_try(stmt.bind("custom"_str, session.custom));
    rstd_try(stmt.bind("cookie"_str, session.cookie));
    rstd_try(stmt.bind("key"_str, session.session_key));
    rstd_try(stmt.bind("now"_str, current_millis() / i64(1000) * i64(1000)));
    return Ok(empty {});
}
template<class Database>
auto revision(Database& db, i64 id) -> ProcessResult<i64> {
    auto row = rstd_try((liorm::sqlito::fetch_optional_scalar<revision_query, Option<i64>>(
        db, liorm::parameters(liorm::param<"id">(id)), liorm::sqlito::RowConsumption::First)));
    return Ok(row.is_some() ? rstd::move(*row).unwrap_or(i64()) : i64());
}
} // namespace qcm::backend::provider_store

export namespace qcm::backend
{
auto save_provider_session(sqlito::Connection& db, i64 provider_id, const String& session_key,
                           const String& custom, const String& cookie) -> ProviderResult<bool> {
    auto query = rstd_try(liorm::sqlito::prepare(db, provider_store::session_save_query));
    rstd_try(query.bind("id"_str, provider_id));
    rstd_try(query.bind("key"_str, session_key));
    rstd_try(query.bind("custom"_str, custom));
    rstd_try(query.bind("cookie"_str, cookie));
    rstd_try(liorm::sqlito::discard(query));
    return Ok(rstd_try(db.last_changes()) == u64(1));
}
auto insert_provider(sqlito::Connection& db, ProviderCreate input) -> ProcessResult<i64> {
    auto tx     = rstd_try(db.begin());
    auto insert = rstd_try(liorm::sqlito::prepare(tx, provider_store::create_query));
    rstd_try(insert.bind("name"_str, input.name));
    rstd_try(insert.bind("type"_str, input.type_name));
    rstd_try(provider_store::bind_session(insert, input.session));
    auto row = rstd_try(
        liorm::sqlito::optional_scalar<i64>(insert, usize(), liorm::sqlito::RowConsumption::Drain));
    if (row.is_none()) return Err(ProcessError::internal("missing provider insert id"_str));
    rstd_try(tx.commit());
    return Ok(*row);
}
auto read_provider_identity(sqlito::Connection& db, i64 id) -> ProcessResult<ProviderIdentity> {
    auto row = rstd_try(liorm::sqlito::fetch_one<provider_store::identity_query>(
        db,
        provider_store::identity_projection,
        liorm::parameters(liorm::param<"id">(id)),
        liorm::sqlito::RowConsumption::First));
    if (row.is_none()) return Err(ProcessError::NoSuchProvider(rstd::format("{}", id)));
    return Ok(ProviderIdentity { row->type_name.unwrap_or(String::make()),
                                 row->session_key.unwrap_or(String::make()) });
}
auto read_provider_settings(sqlito::Connection& db, i64 id)
    -> ProcessResult<ProviderStoredSettings> {
    auto row = rstd_try(liorm::sqlito::fetch_one<provider_store::settings_query>(
        db,
        provider_store::settings_projection,
        liorm::parameters(liorm::param<"id">(id)),
        liorm::sqlito::RowConsumption::First));
    if (row.is_none()) return Err(ProcessError::NoSuchProvider(rstd::format("{}", id)));
    return Ok(ProviderStoredSettings { row->name.unwrap_or(String::make()),
                                       row->config.unwrap_or(String::make("{}"_str)),
                                       row->revision.unwrap_or(i64()) });
}
auto update_provider_record(sqlito::Connection& db, SyncCoordinator& coordinator,
                            ProviderUpdate input) -> ProcessResult<Option<i64>> {
    auto guard = rstd_try(coordinator.write(input.provider_id));
    auto tx    = rstd_try(db.begin());
    if (input.replacement.is_some()) {
        auto update = rstd_try(liorm::sqlito::prepare(tx, provider_store::replace_query));
        rstd_try(update.bind("id"_str, input.provider_id));
        rstd_try(update.bind("name"_str, input.name));
        rstd_try(update.bind("previous"_str, input.session_key));
        rstd_try(provider_store::bind_session(update, *input.replacement));
        rstd_try(liorm::sqlito::discard(update));
    } else {
        auto update = rstd_try(liorm::sqlito::prepare(tx, provider_store::rename_query));
        rstd_try(update.bind("id"_str, input.provider_id));
        rstd_try(update.bind("name"_str, input.name));
        rstd_try(update.bind("previous"_str, input.session_key));
        rstd_try(liorm::sqlito::discard(update));
    }
    if (rstd_try(tx.last_changes()) != u64(1)) return Err(ProcessError::NotAuth());
    auto current = rstd_try(provider_store::revision(tx, input.provider_id));
    if (current != i64(static_cast<long long>(input.settings.revision.to_primitive())))
        return Ok(None<i64>());
    auto revision = current + i64(1);
    auto settings = rstd_try(liorm::sqlito::prepare(tx, provider_store::settings_upsert));
    rstd_try(settings.bind("id"_str, input.provider_id));
    rstd_try(settings.bind("revision"_str, revision));
    rstd_try(settings.bind("config"_str, input.settings.config));
    rstd_try(liorm::sqlito::discard(settings));
    rstd_try(tx.commit());
    if (input.replacement.is_some()) {
        guard->invalidate_sync();
        coordinator.invalidate(input.provider_id);
    }
    return Ok(Some(revision));
}
auto delete_provider_record(sqlito::Connection& db, SyncCoordinator& coordinator, i64 id)
    -> ProcessResult<empty> {
    auto guard  = rstd_try(coordinator.write(id));
    auto tx     = rstd_try(db.begin());
    auto remove = rstd_try(liorm::sqlito::prepare(tx, provider_store::delete_query));
    rstd_try(remove.bind("id"_str, id));
    rstd_try(liorm::sqlito::discard(remove));
    rstd_try(tx.commit());
    guard->invalidate_sync();
    coordinator.invalidate(id);
    return Ok(empty {});
}
} // namespace qcm::backend
