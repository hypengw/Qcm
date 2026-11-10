module;
#include <rstd/enum.hpp>
export module qcm.backend:error;
export import :provider.error;
export import :radio.error;
export import :resource.error;
export import :db.error.query;
import rstd.log;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
// clang-format off
[[pmacro::invoke(
    "rstd::Enum",
    [[derive(Debug, Error)]] enum ProcessError {
        [[error("Unsupported protocol version {version}; expected 2")]]
        ProtocolVersion(version : u32),
        [[error("Backend protocol handshake is not complete")]]
        ProtocolNotReady,
        [[error("{cause}")]]
        Radio([[source]] cause : RadioError),
        [[error("{cause}")]]
        Resource([[source]] cause : resource_cache::Error),
        [[error("Not auth")]]
        NotAuth,
        [[error("{message}")]]
        AuthRequired {
            message: String,
            jump_url: Option<String>,
        },
        [[error("{cause}")]]
        Internal([[source]] cause : ErrorCause),
        [[error("Encode error: {cause}")]]
        Encode([[source]] cause : ErrorCause),
        [[error("Decode error: {cause}")]]
        Decode([[source]] cause : ErrorCause),
        [[error("Unsupported message type: {value}")]]
        UnsupportedMessageType(value : i32),
        [[error("Unknown message type: {value}")]]
        UnknownMessageType(value : i32),
        [[error("Unexpected payload for message type: {value}")]]
        UnexpectedPayload(value : i32),
        [[error("Missing fields: {value}")]]
        MissingFields(value : String),
        [[error("No such provider type: {value}")]]
        NoSuchProviderType(value : String),
        [[error("Wrong id: {value}")]]
        WrongId(value : String),
        [[error("No such library: {value}")]]
        NoSuchLibrary(value : String),
        [[error("No such provider: {value}")]]
        NoSuchProvider(value : String),
        [[error("No such album: {value}")]]
        NoSuchAlbum(value : String),
        [[error("No such song: {value}")]]
        NoSuchSong(value : String),
        [[error("No such artist: {value}")]]
        NoSuchArtist(value : String),
        [[error("No such mix: {value}")]]
        NoSuchMix(value : String),
        [[error("No such item type: {value}")]]
        NoSuchItemType(value : String),
        [[error("No such image type: {value}")]]
        NoSuchImageType(value : String),
        [[error("No such search type: {value}")]]
        NoSuchSearchType(value : String),
        [[error("Unsupported item type: {value}")]]
        UnsupportedItemType(value : String),
        [[error("parse error: {value}")]]
        ParseSubtitle(value : String),
        [[error("Database error: {cause}")]]
        Db([[source]] cause : ErrorCause),
        [[error("HTTP body error: {cause}")]]
        HttpBody([[source]] cause : ErrorCause),
        [[error("Not Found")]]
        NotFound,
        [[error("Not Implemented")]]
        NotImplemented,
    } members {
    public:
        ProcessError(ProviderError error);
        ProcessError(Arc<ProviderError> error);
        ProcessError(sqlito::Error error);
        ProcessError(resource_cache::Error error);
        ProcessError(RadioError error);
        ProcessError(liorm::BindError<sqlito::Error> error);
        ProcessError(QueryError error);
        static auto query(QueryError error) -> ProcessError;
        static auto binding(liorm::BindError<sqlito::Error> error) -> ProcessError;
        static auto internal(ref<str> message) -> ProcessError;
        template<class E>
            requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
        static auto internal(E && error)->ProcessError;
        template<class E>
            requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
        static auto encode(E && error)->ProcessError;
        template<class E>
            requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
        static auto decode(E && error)->ProcessError;
        template<class E>
            requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
        static auto http_body(E && error)->ProcessError;
        template<class E>
            requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
        static auto db(E && error)->ProcessError;
        static auto resource(resource_cache::Error error) -> ProcessError;
        static auto from_provider(ProviderError error) -> ProcessError;
        static auto from_provider(Arc<ProviderError> error) -> ProcessError;
        static auto from_resource(resource_cache::Error error) -> ProcessError;
        static auto radio(RadioError error) -> ProcessError;
    })]];
// clang-format on
inline ProcessError::ProcessError(ProviderError error)
    : ProcessError(from_provider(rstd::move(error))) {}
inline ProcessError::ProcessError(Arc<ProviderError> error)
    : ProcessError(from_provider(rstd::move(error))) {}
inline ProcessError::ProcessError(sqlito::Error error): ProcessError(db(rstd::move(error))) {}
inline ProcessError::ProcessError(resource_cache::Error error)
    : ProcessError(from_resource(rstd::move(error))) {}
inline ProcessError::ProcessError(RadioError error): ProcessError(radio(rstd::move(error))) {}
inline ProcessError::ProcessError(liorm::BindError<sqlito::Error> error)
    : ProcessError(binding(rstd::move(error))) {}
inline ProcessError::ProcessError(QueryError error): ProcessError(query(rstd::move(error))) {}
inline auto ProcessError::query(QueryError error) -> ProcessError {
    return Db(error_cause(rstd::move(error)));
}
inline auto ProcessError::binding(liorm::BindError<sqlito::Error> error) -> ProcessError {
    return Db(error_cause(rstd::move(error)));
}
inline auto ProcessError::internal(ref<str> message) -> ProcessError {
    return Internal(error_cause(message));
}
template<class E>
    requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
auto ProcessError::internal(E&& error) -> ProcessError {
    return Internal(error_cause(rstd::forward<E>(error)));
}
template<class E>
    requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
auto ProcessError::encode(E&& error) -> ProcessError {
    return Encode(error_cause(rstd::forward<E>(error)));
}
template<class E>
    requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
auto ProcessError::decode(E&& error) -> ProcessError {
    return Decode(error_cause(rstd::forward<E>(error)));
}
template<class E>
    requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
auto ProcessError::http_body(E&& error) -> ProcessError {
    return HttpBody(error_cause(rstd::forward<E>(error)));
}
template<class E>
    requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
auto ProcessError::db(E&& error) -> ProcessError {
    return Db(error_cause(rstd::forward<E>(error)));
}
inline auto ProcessError::resource(resource_cache::Error error) -> ProcessError {
    return Resource(rstd::move(error));
}
inline auto ProcessError::from_provider(ProviderError error) -> ProcessError {
    return from_provider(Arc<ProviderError>::make(rstd::move(error)));
}
inline auto ProcessError::from_provider(Arc<ProviderError> error) -> ProcessError {
    const auto& root = error->root();
    if (error->is_WithContext()) {
        if (root.is_NotFound())
            rstd::log::debug("{}", *error);
        else
            rstd::log::error("{}", *error);
    }
    RSTD_MATCH(root) {
        RSTD_CASE(NotFound) { return NotFound(); }
        RSTD_CASE(NotAuth) { return NotAuth(); }
        RSTD_CASE(AuthRequired, message, jump_url) {
            return AuthRequired(message.clone(), jump_url.clone());
        }
        RSTD_CASE(ParseSubtitle, message) { return ParseSubtitle(message.clone()); }
        RSTD_CASE_DEFAULT { return internal(rstd::move(error)); }
    }
}
inline auto ProcessError::from_resource(resource_cache::Error error) -> ProcessError {
    return resource(rstd::move(error));
}
inline auto ProcessError::radio(RadioError error) -> ProcessError {
    RSTD_MATCH(error) {
        RSTD_CASE(Provider, cause) { return from_provider(rstd::move(cause)); }
        RSTD_CASE_DEFAULT { return Radio(rstd::move(error)); }
    }
}
static_assert(! rstd::mtp::copy<ProcessError>);
static_assert(! rstd::Impled<ProcessError, rstd::clone::Clone>);
template<class T>
using ProcessResult = Result<T, ProcessError>;
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<convert::From<qcm::backend::QueryError>, qcm::backend::ProcessError> {
    static auto from(qcm::backend::QueryError error) -> qcm::backend::ProcessError {
        return qcm::backend::ProcessError::query(rstd::move(error));
    }
};
template<>
struct Impl<convert::From<liorm::BindError<sqlito::Error>>, qcm::backend::ProcessError> {
    static auto from(liorm::BindError<sqlito::Error> error) -> qcm::backend::ProcessError {
        return qcm::backend::ProcessError::binding(rstd::move(error));
    }
};
template<>
struct Impl<convert::From<qcm::backend::ProviderError>, qcm::backend::ProcessError> {
    static auto from(qcm::backend::ProviderError error) -> qcm::backend::ProcessError {
        return qcm::backend::ProcessError(rstd::move(error));
    }
};
template<>
struct Impl<convert::From<sync::Arc<qcm::backend::ProviderError>>, qcm::backend::ProcessError> {
    static auto from(sync::Arc<qcm::backend::ProviderError> error) -> qcm::backend::ProcessError {
        return qcm::backend::ProcessError(rstd::move(error));
    }
};
template<>
struct Impl<convert::From<sqlito::Error>, qcm::backend::ProcessError> {
    static auto from(sqlito::Error error) -> qcm::backend::ProcessError {
        return qcm::backend::ProcessError(rstd::move(error));
    }
};
template<>
struct Impl<convert::From<qcm::backend::resource_cache::Error>, qcm::backend::ProcessError> {
    static auto from(qcm::backend::resource_cache::Error error) -> qcm::backend::ProcessError {
        return qcm::backend::ProcessError(rstd::move(error));
    }
};
template<>
struct Impl<convert::From<qcm::backend::RadioError>, qcm::backend::ProcessError> {
    static auto from(qcm::backend::RadioError error) -> qcm::backend::ProcessError {
        return qcm::backend::ProcessError(rstd::move(error));
    }
};
} // namespace rstd
