module;
#include <rstd/macro.hpp>
export module qcm.backend:plugin.state;
import :db.database;
import liorm.sqlito;
export import sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
using SqlStep           = sqlito::Step;
using SqlExecutorHandle = sqlito::ExecutorHandle;
namespace q             = liorm::query;
namespace s             = liorm::schema;

export namespace qcm::backend
{
// clang-format off
[[pmacro::invoke(
    "rstd::Enum",
    [[derive(Debug, Error)]] enum StateErrorReason {
        [[error("invalid plugin state namespace")]]
        InvalidNamespace,
        [[error("invalid plugin installation")]]
        InvalidInstallation,
        [[error("plugin state exceeds size limit")]]
        TooLarge,
        [[error("plugin state revision exhausted")]]
        RevisionExhausted,
        [[error("invalid plugin state row")]]
        InvalidRow,
        [[error("{error}")]]
        Storage([[source]] error : sqlito::Error),
    })]];
// clang-format on
struct [[pmacro::derive("rstd::Debug")]] StateError {
    StateErrorReason reason;
    bool             outcome_unknown {};
};
template<class T>
using StateResult = Result<T, StateError>;
struct InstallationState {
    u64            revision {};
    Option<String> data;
};
struct StateUpdate {
    bool              applied {};
    InstallationState state;
};
inline constexpr usize max_state_bytes = usize(1024 * 1024);
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<fmt::Display, qcm::backend::StateError> : ImplBase<qcm::backend::StateError> {
    auto fmt(fmt::Formatter& f) const -> bool {
        const auto& error = this->self();
        if (error.outcome_unknown && ! f.write_str("plugin state write outcome unknown: "_str))
            return false;
        return f.write_fmt(fmt::Arguments::make("{}", error.reason));
    }
};
template<>
struct Impl<error::Error, qcm::backend::StateError>
    : DefaultInImpl<error::Error, qcm::backend::StateError> {
    auto source() const noexcept -> Option<error::ErrorRef> {
        return Some(dyn<error::Error>::from_ref(this->self().reason));
    }
};
} // namespace rstd

namespace qcm::backend
{
using sqlito::Connection;
template<class T>
using DbResult = Result<StateResult<T>, sqlito::Error>;
auto domain(StateErrorReason reason) -> StateError { return { rstd::move(reason) }; }
auto storage(sqlito::Error e, bool outcome_unknown = false) -> StateError {
    return { StateErrorReason::Storage(rstd::move(e)), outcome_unknown };
}
struct StateFields {
    Option<i64>    revision;
    Option<String> data;
};
inline constexpr liorm::RecordProjection<StateFields, &StateFields::revision, &StateFields::data>
                      state_projection;
inline constexpr auto schema       = s::define([](s::SchemaBuilder& b) {
    auto t  = b.table("plugin_installation_state"_str);
    auto ns = t.column<q::TextParameter>("namespace"_str);
    t.column<i64>("revision"_str);
    t.column<q::TextParameter>("data"_str);
    t.primary_key("plugin_state_pk"_str, { ns });
});
inline constexpr auto read_query   = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(schema).table("plugin_installation_state"_str);
    return state_projection.select(b, t.col("revision"_str), t.col("data"_str))
        .from(t.source())
        .where(t.col("namespace"_str) == b.param<q::TextParameter>("namespace"_str));
});
inline constexpr auto insert_query = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(schema).table("plugin_installation_state"_str);
    return b
        .insert_into(t.source(), t.col("namespace"_str), t.col("revision"_str), t.col("data"_str))
        .values(b.param<q::TextParameter>("namespace"_str),
                b.integer(1),
                b.param<q::TextParameter>("data"_str))
        .on_conflict_do_nothing(t.col("namespace"_str))
        .returning(t.col("revision"_str), t.col("data"_str));
});
inline constexpr auto update_query = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(schema).table("plugin_installation_state"_str);
    return b.update(t.source())
        .set(t.col("revision"_str).set(t.col("revision"_str) + b.integer(1)),
             t.col("data"_str).set(b.param<q::TextParameter>("data"_str)))
        .where((t.col("namespace"_str) == b.param<q::TextParameter>("namespace"_str)) &&
               (t.col("revision"_str) == b.param<i64>("revision"_str)))
        .returning(t.col("revision"_str), t.col("data"_str));
});
static_assert(
    read_query.sql() ==
    "SELECT \"plugin_installation_state\".\"revision\", \"plugin_installation_state\".\"data\" FROM \"plugin_installation_state\" WHERE (\"plugin_installation_state\".\"namespace\" = ?1)"_str);
static_assert(
    insert_query.sql() ==
    "INSERT INTO \"plugin_installation_state\" (\"namespace\", \"revision\", \"data\") VALUES (?1, 1, ?2) ON CONFLICT (\"namespace\") DO NOTHING RETURNING \"revision\", \"data\""_str);
static_assert(
    update_query.sql() ==
    "UPDATE \"plugin_installation_state\" SET \"revision\" = (\"plugin_installation_state\".\"revision\" + 1), \"data\" = ?1 WHERE ((\"plugin_installation_state\".\"namespace\" = ?2) AND (\"plugin_installation_state\".\"revision\" = ?3)) RETURNING \"revision\", \"data\""_str);
static_assert(read_query.parameter_slot("namespace"_str).unwrap() == usize(1));
static_assert(insert_query.parameter_slot("namespace"_str).unwrap() == usize(1));
static_assert(insert_query.parameter_slot("data"_str).unwrap() == usize(2));
static_assert(update_query.parameter_slot("data"_str).unwrap() == usize(1));
static_assert(update_query.parameter_slot("namespace"_str).unwrap() == usize(2));
static_assert(update_query.parameter_slot("revision"_str).unwrap() == usize(3));
auto valid_namespace(ref<str> ns) -> bool {
    if (ns.len() == usize() || ns.len() > usize(128)) return false;
    for (auto b : ns.as_bytes()) {
        auto c = b.to_primitive();
        if (! ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
               c == '.' || c == '_' || c == '-'))
            return false;
    }
    return true;
}
auto decode(sqlito::Statement& stmt) -> DbResult<InstallationState> {
    liorm::sqlito::Reader reader(stmt);
    auto                  fields   = rstd_try(state_projection.decode(reader));
    auto&                 revision = fields.revision;
    auto&                 data     = fields.data;
    if (revision.is_none() || data.is_none() || *revision <= i64() || data->len() > max_state_bytes)
        return Ok(StateResult<InstallationState>(Err(domain(StateErrorReason::InvalidRow()))));
    return Ok(StateResult<InstallationState>(
        Ok(InstallationState { u64(revision->to_primitive()), Some(rstd::move(data).unwrap()) })));
}
auto read(Connection& db, ref<str> ns) -> DbResult<InstallationState> {
    auto stmt = rstd_try(db.prepare(read_query.sql()));
    rstd_try(stmt.bind_text(read_query.parameter_slot("namespace"_str).unwrap(), ns));
    auto step = rstd_try(stmt.step());
    if (step == SqlStep::Done) return Ok(StateResult<InstallationState>(Ok(InstallationState {})));
    auto value = rstd_try(decode(stmt));
    auto end   = rstd_try(stmt.step());
    if (end != SqlStep::Done)
        return Ok(StateResult<InstallationState>(Err(domain(StateErrorReason::InvalidRow()))));
    return Ok(rstd::move(value));
}
auto write(Connection& db, ref<str> ns, u64 revision, ref<str> data)
    -> DbResult<Option<InstallationState>> {
    const auto& query = revision == u64() ? insert_query : update_query;
    auto        stmt  = rstd_try(db.prepare(query.sql()));
    rstd_try(stmt.bind_text(query.parameter_slot("namespace"_str).unwrap(), ns));
    rstd_try(stmt.bind_text(query.parameter_slot("data"_str).unwrap(), data));
    if (revision != u64())
        rstd_try(
            stmt.bind(query.parameter_slot("revision"_str).unwrap(), i64(revision.to_primitive())));
    auto step = rstd_try(stmt.step());
    if (step == SqlStep::Done)
        return Ok(StateResult<Option<InstallationState>>(Ok(Option<InstallationState> {})));
    auto decoded = rstd_try(decode(stmt));
    auto end     = rstd_try(stmt.step());
    if (end != SqlStep::Done) {
        auto error            = domain(StateErrorReason::InvalidRow());
        error.outcome_unknown = true;
        return Ok(StateResult<Option<InstallationState>>(Err(rstd::move(error))));
    }
    if (decoded.is_err()) {
        auto error            = rstd::move(decoded).unwrap_err();
        error.outcome_unknown = true;
        return Ok(StateResult<Option<InstallationState>>(Err(rstd::move(error))));
    }
    return Ok(StateResult<Option<InstallationState>>(Ok(Some(rstd::move(decoded).unwrap()))));
}
auto load_async(SqlExecutorHandle handle, String ns)
    -> rstd::async::coro<StateResult<InstallationState>> {
    auto operation = rstd_co_try(handle.submit([ns = rstd::move(ns)](Connection& db) {
        return read(db, ns.as_str());
    }),
                                 [](auto error) {
                                     return storage(rstd::move(error));
                                 });
    co_return rstd_co_try(co_await rstd::move(operation), [](auto error) {
        return storage(rstd::move(error));
    });
}
auto cas_async(SqlExecutorHandle handle, String ns, u64 revision, String data)
    -> rstd::async::coro<StateResult<StateUpdate>> {
    if (data.len() > max_state_bytes) co_return Err(domain(StateErrorReason::TooLarge()));
    if (revision >= u64(9223372036854775807ULL))
        co_return Err(domain(StateErrorReason::RevisionExhausted()));
    auto op = rstd_co_try(
        handle.submit(
            [ns = String::make(ns.as_str()), revision, data = rstd::move(data)](Connection& db) {
                return write(db, ns.as_str(), revision, data.as_str());
            }),
        [](auto error) {
            return storage(rstd::move(error));
        });
    auto result = rstd_co_try(co_await rstd::move(op), [](auto error) {
        return storage(rstd::move(error), true);
    });
    auto state  = rstd_co_try(result);
    if (state.is_some()) co_return Ok(StateUpdate { true, rstd::move(state).unwrap() });
    auto current = rstd_co_try(co_await load_async(handle.clone(), rstd::move(ns)));
    co_return Ok(StateUpdate { false, rstd::move(current) });
}
} // namespace qcm::backend
export namespace qcm::backend
{
auto load_state(sqlito::Connection& db, ref<str> ns) -> StateResult<InstallationState> {
    if (! valid_namespace(ns)) return Err(domain(StateErrorReason::InvalidNamespace()));
    auto outer = read(db, ns);
    if (outer.is_err()) return Err(storage(rstd::move(outer).unwrap_err()));
    auto inner = rstd::move(outer).unwrap();
    if (inner.is_err()) return Err(rstd::move(inner).unwrap_err());
    return Ok(rstd::move(inner).unwrap());
}
auto store_state(sqlito::Connection& db, ref<str> ns, u64 revision, String data)
    -> StateResult<StateUpdate> {
    if (! valid_namespace(ns)) return Err(domain(StateErrorReason::InvalidNamespace()));
    if (data.len() > max_state_bytes) return Err(domain(StateErrorReason::TooLarge()));
    if (revision >= u64(9223372036854775807ULL))
        return Err(domain(StateErrorReason::RevisionExhausted()));
    auto outer = write(db, ns, revision, data.as_str());
    if (outer.is_err()) return Err(storage(rstd::move(outer).unwrap_err(), true));
    auto inner = rstd::move(outer).unwrap();
    if (inner.is_err()) return Err(rstd::move(inner).unwrap_err());
    auto updated = rstd::move(inner).unwrap();
    if (updated.is_some()) return Ok(StateUpdate { true, rstd::move(updated).unwrap() });
    auto current = load_state(db, ns);
    if (current.is_err()) return Err(rstd::move(current).unwrap_err());
    return Ok(StateUpdate { false, rstd::move(current).unwrap() });
}

class PluginState {
    SqlExecutorHandle handle_;
    String            namespace_;
    String            installation_;
    PluginState(SqlExecutorHandle h, String ns, String installation)
        : handle_(rstd::move(h)),
          namespace_(rstd::move(ns)),
          installation_(rstd::move(installation)) {}

public:
    auto clone() const -> PluginState {
        return PluginState(handle_.clone(), namespace_.clone(), installation_.clone());
    }
    auto        namespace_name() const -> ref<str> { return namespace_.as_str(); }
    auto        installation_id() const -> ref<str> { return installation_.as_str(); }
    static auto create(SqlExecutorHandle h, String ns, String installation)
        -> StateResult<PluginState> {
        if (! valid_namespace(ns.as_str()))
            return Err(domain(StateErrorReason::InvalidNamespace()));
        if (installation.len() == usize())
            return Err(domain(StateErrorReason::InvalidInstallation()));
        return Ok(PluginState(rstd::move(h), rstd::move(ns), rstd::move(installation)));
    }

    auto load() const -> rstd::async::coro<StateResult<InstallationState>> {
        return load_async(handle_.clone(), String::make(namespace_.as_str()));
    }
    // Any error with outcome_unknown may follow a completed write; load again to verify.
    auto compare_exchange(u64 revision, String data) const
        -> rstd::async::coro<StateResult<StateUpdate>> {
        return cas_async(
            handle_.clone(), String::make(namespace_.as_str()), revision, rstd::move(data));
    }
};
class StateDatabase {
    MainDatabase database_;
    explicit StateDatabase(MainDatabase database): database_(rstd::move(database)) {}

public:
    StateDatabase(StateDatabase&&)      = default;
    StateDatabase(const StateDatabase&) = delete;
    static auto open(String path) -> rstd::async::coro<StateResult<StateDatabase>> {
        auto opened = rstd_co_try(co_await MainDatabase::open(rstd::move(path)), [](auto error) {
            return storage(rstd::move(error));
        });
        co_return Ok(StateDatabase(rstd::move(opened)));
    }
    auto handle() const -> StateResult<SqlExecutorHandle> {
        auto result = database_.handle();
        if (result.is_err()) return Err(storage(rstd::move(result).unwrap_err()));
        return Ok(rstd::move(result).unwrap());
    }
    auto plugin(String ns, String installation) const -> StateResult<PluginState> {
        auto h = handle();
        if (h.is_err()) return Err(rstd::move(h).unwrap_err());
        return PluginState::create(
            rstd::move(h).unwrap(), rstd::move(ns), rstd::move(installation));
    }
    auto shutdown() -> rstd::async::coro<Result<empty, sqlito::Error>> {
        return database_.shutdown();
    }
};
} // namespace qcm::backend
