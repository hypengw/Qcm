module;
#include <rstd/enum.hpp>
export module qcm.backend:resource.error;
export import rstd;
export import lifetchto;
export import sqlito;
export import lihttpto;
export import :provider.error;
using namespace rstd::prelude;
using namespace rstd::literals;
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

inline Error::Error(sqlito::Error cause): Error(storage(rstd::move(cause))) {}
inline Error::Error(rstd::io::Error cause): Error(io(rstd::move(cause))) {}
inline Error::Error(ProviderError cause): Error(provider(rstd::move(cause))) {}
inline auto Error::request(lifetchto::Error cause) -> Error {
    return provider(ProviderError::request(rstd::move(cause)));
}
inline auto Error::provider(ProviderError cause) -> Error {
    return Provider(Arc<ProviderError>::make(rstd::move(cause)));
}
inline auto Error::storage(sqlito::Error cause) -> Error {
    return Storage(Arc<sqlito::Error>::make(rstd::move(cause)));
}
inline auto Error::io(rstd::io::Error cause) -> Error {
    return Io(Arc<rstd::io::Error>::make(rstd::move(cause)));
}
inline auto Error::invalidated() -> Error { return Invalidated(); }
inline auto Error::changed() -> Error { return Changed(); }
inline auto Error::plain_corrupt() -> Error { return PlainCorrupt(); }
inline auto Error::upstream(i32 status, lihttpto::Headers headers) -> Error {
    return Upstream(status, rstd::move(headers));
}
inline auto Error::state(ref<str> message) -> Error { return State(String::make(message), None()); }
template<class E>
    requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
auto Error::state(ref<str> message, E&& cause) -> Error {
    return State(String::make(message),
                 Some(Arc<Cause>::make(
                     Cause { Box<dyn<rstd::error::Error>>::make(rstd::forward<E>(cause)) })));
}

inline auto Error::retryable_request() const -> bool {
    RSTD_MATCH(*this) {
        RSTD_CASE(Provider, provider) {
            const ProviderError& error = *provider;
            RSTD_MATCH(error) {
                RSTD_CASE(Request, cause) {
                    return cause.is_Resolve() || cause.is_Connect() || cause.is_Timeout();
                }
                RSTD_CASE_DEFAULT { return false; }
            }
        }
        RSTD_CASE_DEFAULT { return false; }
    }
}
inline auto Error::should_reopen() const -> bool { return is_Changed() || is_PlainCorrupt(); }
inline auto Error::clone() const -> Error {
    RSTD_MATCH(*this) {
        RSTD_CASE(Provider, cause) { return Provider(cause.clone()); }
        RSTD_CASE(Storage, cause) { return Storage(cause.clone()); }
        RSTD_CASE(Io, cause) { return Io(cause.clone()); }
        RSTD_CASE(Invalidated) { return Invalidated(); }
        RSTD_CASE(Changed) { return Changed(); }
        RSTD_CASE(PlainCorrupt) { return PlainCorrupt(); }
        RSTD_CASE(Upstream, status, headers) { return Upstream(status, headers.clone()); }
        RSTD_CASE(State, message, cause) {
            return State(message.clone(), cause.is_some() ? Some(cause->clone()) : None());
        }
    }
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
