export module qcm.backend:db.provider.store;
export import :error;
export import :db.provider.schema;
export import :db.sync.run;
export import sqlito;
using namespace rstd::prelude;

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
auto save_provider_session(sqlito::Connection& db, i64 provider_id, const String& session_key,
                           const String& custom, const String& cookie) -> ProviderResult<bool>;
auto insert_provider(sqlito::Connection& db, ProviderCreate input) -> ProcessResult<i64>;
auto read_provider_identity(sqlito::Connection& db, i64 id) -> ProcessResult<ProviderIdentity>;
auto read_provider_settings(sqlito::Connection& db, i64 id)
    -> ProcessResult<ProviderStoredSettings>;
auto update_provider_record(sqlito::Connection& db, SyncCoordinator& coordinator,
                            ProviderUpdate input) -> ProcessResult<Option<i64>>;
auto delete_provider_record(sqlito::Connection& db, SyncCoordinator& coordinator, i64 id)
    -> ProcessResult<empty>;
} // namespace qcm::backend
