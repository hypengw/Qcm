export module qcm.backend:provider.error;
export import :error_cause;
export import lifetchto;
export import sqlito;
export import liorm;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
// clang-format off
[[pmacro::invoke(
    "rstd::Enum",
    [[derive(Debug, Error)]] enum ProviderError {
        [[error("{cause}")]]
        Internal([[source]] cause : ErrorCause),
        [[error("{cause}")]]
        Io([[from]] cause : rstd::io::Error),
        [[error("{cause}")]]
        Serde([[source]] cause : ErrorCause),
        [[error("parse error: {message}")]]
        ParseSubtitle(message : String),
        [[error("[lua] {message}")]]
        Lua(message : String),
        [[error("{cause}")]]
        External([[source]] cause : Arc<ErrorCause>),
        [[error("{error}\n{context}")]]
        WithContext {
            context: String,
            [[source]] error: Arc<ProviderError>,
        },
        [[error("Not auth")]]
        NotAuth,
        [[error("{message}")]]
        AuthRequired {
            message: String,
            jump_url: Option<String>,
        },
        [[error("Unknown base")]]
        UnknownBase,
        [[error("Request error: {cause}")]]
        Request([[from]] cause : lifetchto::Error),
        [[error("UUid: {cause}")]]
        Uuid([[source]] cause : ErrorCause),
        [[error("Not Found")]]
        NotFound,
        [[error("Not Implemented")]]
        NotImplemented,
        [[error("Database error: {cause}")]]
        Db([[source]] cause : ErrorCause),
    } members {
    public:
        ProviderError(sqlito::Error error);
        ProviderError(rstd::io::Error error);
        ProviderError(lifetchto::Error error);
        ProviderError(liorm::BindError<sqlito::Error> error);
        static auto binding(liorm::BindError<sqlito::Error> error) -> ProviderError;
        static auto internal(ref<str> message) -> ProviderError;
        template<class E>
            requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
        static auto internal(E && error)->ProviderError;
        template<class E>
            requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
        static auto external(E && error)->ProviderError;
        template<class E>
            requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
        static auto serde(E && error)->ProviderError;
        static auto request(lifetchto::Error error) -> ProviderError;
        static auto io(rstd::io::Error error) -> ProviderError;
        template<class E>
            requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
        static auto db(E && error)->ProviderError;
        auto        root() const& -> const ProviderError&;
        auto        root() const&& -> const ProviderError& = delete;
        auto        with_context(ref<str> context) && -> ProviderError;
    })]];
// clang-format on
template<class E>
    requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
auto ProviderError::internal(E&& error) -> ProviderError {
    return Internal(error_cause(rstd::forward<E>(error)));
}
template<class E>
    requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
auto ProviderError::external(E&& error) -> ProviderError {
    return External(Arc<ErrorCause>::make(error_cause(rstd::forward<E>(error))));
}
template<class E>
    requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
auto ProviderError::serde(E&& error) -> ProviderError {
    return Serde(error_cause(rstd::forward<E>(error)));
}
template<class E>
    requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
auto ProviderError::db(E&& error) -> ProviderError {
    return Db(error_cause(rstd::forward<E>(error)));
}
static_assert(! rstd::mtp::copy<ProviderError>);
static_assert(! rstd::Impled<ProviderError, rstd::clone::Clone>);
template<class T>
using ProviderResult = Result<T, ProviderError>;
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<convert::From<liorm::BindError<sqlito::Error>>, qcm::backend::ProviderError> {
    static auto from(liorm::BindError<sqlito::Error> error) -> qcm::backend::ProviderError;
};
template<>
struct Impl<convert::From<sqlito::Error>, qcm::backend::ProviderError> {
    static auto from(sqlito::Error error) -> qcm::backend::ProviderError;
};
} // namespace rstd
