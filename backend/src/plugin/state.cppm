export module qcm.backend:plugin.state;
import :db.database;
export import sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
using SqlExecutorHandle = sqlito::ExecutorHandle;

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
    auto fmt(fmt::Formatter& f) const -> bool;
};
template<>
struct Impl<error::Error, qcm::backend::StateError>
    : DefaultInImpl<error::Error, qcm::backend::StateError> {
    auto source() const noexcept -> Option<error::ErrorRef>;
};
} // namespace rstd

export namespace qcm::backend
{
auto load_state(sqlito::Connection& db, ref<str> ns) -> StateResult<InstallationState>;
auto store_state(sqlito::Connection& db, ref<str> ns, u64 revision, String data)
    -> StateResult<StateUpdate>;

class PluginState {
    SqlExecutorHandle handle_;
    String            namespace_;
    String            installation_;
    PluginState(SqlExecutorHandle h, String ns, String installation);

public:
    auto        clone() const -> PluginState;
    auto        namespace_name() const -> ref<str> { return namespace_.as_str(); }
    auto        installation_id() const -> ref<str> { return installation_.as_str(); }
    static auto create(SqlExecutorHandle h, String ns, String installation)
        -> StateResult<PluginState>;

    auto load() const -> rstd::async::coro<StateResult<InstallationState>>;
    // Any error with outcome_unknown may follow a completed write; load again to verify.
    auto compare_exchange(u64 revision, String data) const
        -> rstd::async::coro<StateResult<StateUpdate>>;
};
class StateDatabase {
    MainDatabase database_;
    explicit StateDatabase(MainDatabase database);

public:
    StateDatabase(StateDatabase&&)      = default;
    StateDatabase(const StateDatabase&) = delete;
    static auto open(String path) -> rstd::async::coro<StateResult<StateDatabase>>;
    auto handle() const -> StateResult<SqlExecutorHandle>;
    auto plugin(String ns, String installation) const -> StateResult<PluginState>;
    auto shutdown() -> rstd::async::coro<Result<empty, sqlito::Error>>;
};
} // namespace qcm::backend
