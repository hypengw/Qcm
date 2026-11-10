export module qcm.backend:db.error.query;
export import liorm;
export import sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
// clang-format off
[[pmacro::invoke(
    "rstd::Enum",
    [[derive(Debug, Error)]] enum QueryError {
        [[error("{error}")]]
        Compile([[from]] error : liorm::query::Error),
        [[error("{error}")]]
        Binding([[from]] error : liorm::BindError<sqlito::Error>),
        [[error("{error}")]]
        Storage([[from]] error : sqlito::Error),
        [[error("{message}")]]
        Invalid(message : String),
    } members {
    public:
        QueryError(sqlito::Error error);
        QueryError(liorm::query::Error error);
        QueryError(liorm::BindError<sqlito::Error> error);
        static auto invalid(ref<str> message) -> QueryError;
    })]];
// clang-format on
inline auto QueryError::invalid(ref<str> message) -> QueryError {
    return Invalid(String::make(message));
}
inline QueryError::QueryError(sqlito::Error error): QueryError(Storage(rstd::move(error))) {}
inline QueryError::QueryError(liorm::query::Error error): QueryError(Compile(rstd::move(error))) {}
inline QueryError::QueryError(liorm::BindError<sqlito::Error> error)
    : QueryError(Binding(rstd::move(error))) {}
template<class T>
using QueryResult = Result<T, QueryError>;
static_assert(! rstd::mtp::copy<QueryError>);
} // namespace qcm::backend
