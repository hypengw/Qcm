module;
#include <rstd/enum.hpp>
#include <rstd/macro.hpp>
export module qcm.backend:provider.lua_error;
export import :provider.error;
export import luato;
using namespace rstd::prelude;
using namespace rstd::literals;
using qcm::backend::ProviderError;

export namespace qcm::backend
{
auto provider_error_to_lua(ProviderError error, ref<str> source = "qcm"_str) -> luato::Error {
    auto message = rstd::format("{}", error);
    return luato::Error::make(luato::ErrorKind::Runtime, String::make(source), rstd::move(message))
        .with_cause(rstd::move(error));
}
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<convert::From<luato::Error>, qcm::backend::ProviderError> {
    static auto from(luato::Error error) -> qcm::backend::ProviderError {
        auto context   = rstd::move(error.traceback);
        auto converted = [&]() -> ProviderError {
            if (auto cause = error.native_cause()) {
                if (auto provider = rstd::error::downcast_ref<ProviderError>(*cause)) {
                    RSTD_MATCH(**provider) {
                        RSTD_CASE(Lua, message) { return ProviderError::Lua(message.clone()); }
                        RSTD_CASE(NotAuth) { return ProviderError::NotAuth(); }
                        RSTD_CASE(AuthRequired, message, jump_url) {
                            return ProviderError::AuthRequired(message.clone(), jump_url.clone());
                        }
                        RSTD_CASE(NotFound) { return ProviderError::NotFound(); }
                        RSTD_CASE(NotImplemented) { return ProviderError::NotImplemented(); }
                        RSTD_CASE_DEFAULT { break; }
                    }
                }
                return ProviderError::external(rstd::move(error));
            }
            return ProviderError::Lua(rstd::move(error.message));
        }();
        if (! context.is_empty()) return rstd::move(converted).with_context(context.as_str());
        return converted;
    }
};
} // namespace rstd
