export module qcm.backend:resource.error;
export import rstd;
export import sqlito;
export import lihttpto;
export import :provider.error;
using namespace rstd::prelude;
using qcm::backend::ProviderError;
using rstd::sync::Arc;

export namespace qcm::backend::resource_cache
{
struct [[pmacro::derive("rstd::Debug"), pmacro::helper("rstd::Debug", tuple)]] Cause {
    Box<dyn<rstd::error::Error>> error;
};
namespace resource_error
{
struct [[pmacro::derive("rstd::Debug")]] State {
    String             message;
    Option<Arc<Cause>> cause;
};
} // namespace resource_error
} // namespace qcm::backend::resource_cache

export namespace rstd
{
template<>
struct Impl<fmt::Display, qcm::backend::resource_cache::resource_error::State>
    : ImplBase<qcm::backend::resource_cache::resource_error::State> {
    auto fmt(fmt::Formatter& f) const -> bool {
        const auto& error = this->self();
        if (error.cause.is_some())
            return f.write_fmt(
                fmt::Arguments::make("{}: {}", error.message, (*error.cause)->error.deref()));
        return f.write_str(error.message.as_str());
    }
};
template<>
struct Impl<error::Error, qcm::backend::resource_cache::resource_error::State>
    : DefaultInImpl<error::Error, qcm::backend::resource_cache::resource_error::State> {
    auto source() const noexcept -> Option<error::ErrorRef> {
        const auto& error = this->self();
        if (error.cause.is_some()) return Some((*error.cause)->error.deref());
        return None();
    }
};
} // namespace rstd

export namespace qcm::backend::resource_cache
{
// clang-format off
[[pmacro::invoke(
    "rstd::Enum",
    [[derive(Debug, Error)]] enum Error {
        [[error("{cause}")]]
        Provider([[source]] cause : Arc<ProviderError>),
        [[error("{cause}")]]
        Storage([[source]] cause : Arc<sqlito::Error>),
        [[error("{cause}")]]
        Io([[source]] cause : Arc<rstd::io::Error>),
        [[error("resource invalidated")]]
        Invalidated,
        [[error("cached representation changed")]]
        Changed,
        [[error("published cache file is corrupt")]]
        PlainCorrupt,
        [[error("upstream resource status {status}")]]
        Upstream {
            status: i32,
            headers: lihttpto::Headers,
        },
        State = resource_error::State,
    } members {
    public:
        Error(sqlito::Error cause);
        Error(rstd::io::Error cause);
        Error(ProviderError cause);
        static auto request(lifetchto::Error cause) -> Error;
        static auto provider(ProviderError cause) -> Error;
        static auto storage(sqlito::Error cause) -> Error;
        static auto io(rstd::io::Error cause) -> Error;
        static auto invalidated() -> Error;
        static auto changed() -> Error;
        static auto plain_corrupt() -> Error;
        static auto upstream(i32 status, lihttpto::Headers headers) -> Error;
        static auto state(ref<str> message) -> Error;
        template<class E>
            requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
        static auto state(ref<str> message, E && cause) -> Error;
        auto        retryable_request() const -> bool;
        auto        should_reopen() const -> bool;
        auto        clone() const -> Error;
    })]];
// clang-format on

template<class E>
    requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
auto Error::state(ref<str> message, E&& cause) -> Error {
    return State(String::make(message),
                 Some(Arc<Cause>::make(
                     Cause { Box<dyn<rstd::error::Error>>::make(rstd::forward<E>(cause)) })));
}

} // namespace qcm::backend::resource_cache

export namespace rstd
{
template<>
struct Impl<convert::From<sqlito::Error>, qcm::backend::resource_cache::Error> {
    static auto from(sqlito::Error cause) -> qcm::backend::resource_cache::Error {
        return qcm::backend::resource_cache::Error::storage(rstd::move(cause));
    }
};
template<>
struct Impl<convert::From<io::Error>, qcm::backend::resource_cache::Error> {
    static auto from(io::Error cause) -> qcm::backend::resource_cache::Error {
        return qcm::backend::resource_cache::Error::io(rstd::move(cause));
    }
};
template<>
struct Impl<convert::From<qcm::backend::ProviderError>, qcm::backend::resource_cache::Error> {
    static auto from(qcm::backend::ProviderError cause) -> qcm::backend::resource_cache::Error {
        return qcm::backend::resource_cache::Error::provider(rstd::move(cause));
    }
};
} // namespace rstd
