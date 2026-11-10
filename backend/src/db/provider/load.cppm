module;
#include <rstd/macro.hpp>
export module qcm.backend:db.provider.load;
export import :db.error.query;
import :db.provider.schema;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace provider_load_query = liorm::query;

export namespace qcm::backend
{
struct ProviderLoadRow {
    i64            id;
    String         type_name;
    String         name;
    String         base_url;
    String         custom;
    Option<String> auth;
    Option<String> cookie;
    String         session_key;
    Option<String> settings;
    u64            settings_revision {};
};
} // namespace qcm::backend

namespace qcm::backend
{
struct ProviderLoadFields {
    Option<i64>    id;
    Option<String> type_name;
    Option<String> name;
    Option<String> base_url;
    Option<String> custom;
    Option<String> auth;
    Option<String> cookie;
    Option<String> session_key;
    Option<String> settings;
    Option<i64>    revision;
};
inline constexpr liorm::RecordProjection<
    ProviderLoadFields, &ProviderLoadFields::id, &ProviderLoadFields::type_name,
    &ProviderLoadFields::name, &ProviderLoadFields::base_url, &ProviderLoadFields::custom,
    &ProviderLoadFields::auth, &ProviderLoadFields::cookie, &ProviderLoadFields::session_key,
    &ProviderLoadFields::settings, &ProviderLoadFields::revision>
    provider_load_projection;
struct ProviderLoadProjection {
    using value_type = ProviderLoadRow;

    auto decode(liorm::sqlito::Reader& reader) const -> QueryResult<ProviderLoadRow> {
        auto fields = rstd_try(provider_load_projection.decode(reader));
        if (fields.revision.is_some() && *fields.revision < i64())
            return Err(QueryError::invalid("negative provider settings revision"_str));
        return Ok(ProviderLoadRow { fields.id.unwrap_or(i64()),
                                    fields.type_name.unwrap_or(String::make()),
                                    fields.name.unwrap_or(String::make()),
                                    fields.base_url.unwrap_or(String::make()),
                                    fields.custom.unwrap_or(String::make()),
                                    rstd::move(fields.auth),
                                    rstd::move(fields.cookie),
                                    fields.session_key.unwrap_or(String::make()),
                                    rstd::move(fields.settings),
                                    fields.revision.is_some() ? u64(fields.revision->to_primitive())
                                                              : u64() });
    }
};
} // namespace qcm::backend

export namespace qcm::backend
{
template<bool Changed>
inline constexpr auto provider_load_statement =
    provider_load_query::compile([](provider_load_query::QueryBuilder& b) {
        auto schema   = b.use(provider_tables);
        auto provider = schema.table<ProviderRecord>();
        auto settings = schema.table<ProviderSettingsRecord>();
        auto select   = provider_load_projection
                            .select(b,
                                    provider.col<&ProviderRecord::provider_id>(),
                                    provider.col<&ProviderRecord::type_name>(),
                                    provider.col<&ProviderRecord::name>(),
                                    provider.col<&ProviderRecord::base_url>(),
                                    provider.col<&ProviderRecord::custom>(),
                                    provider.col<&ProviderRecord::auth_method>(),
                                    provider.col<&ProviderRecord::cookie>(),
                                    provider.col<&ProviderRecord::session_key>(),
                                    settings.col<&ProviderSettingsRecord::config>(),
                                    settings.col<&ProviderSettingsRecord::revision>())
                            .from(provider.source())
                            .left_join(settings.source(),
                                       provider.col<&ProviderRecord::provider_id>() ==
                                           settings.col<&ProviderSettingsRecord::provider_id>());
        if constexpr (Changed)
            select = select.where(provider.col<&ProviderRecord::provider_id>() ==
                                  b.param<i64>("id"_str));
        return select;
    });
auto provider_load_rows(sqlito::Connection& db, Option<i64> changed)
    -> QueryResult<Vec<ProviderLoadRow>> {
    if (changed.is_some())
        return liorm::sqlito::fetch_all<provider_load_statement<true>>(
            db, ProviderLoadProjection {}, liorm::parameters(liorm::param<"id">(*changed)));
    return liorm::sqlito::fetch_all<provider_load_statement<false>>(db, ProviderLoadProjection {});
}
} // namespace qcm::backend
