export module qcm.backend:error_cause;
export import rstd;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct [[pmacro::derive("rstd::Debug")]] ErrorCause {
    String                               message;
    Option<Box<dyn<rstd::error::Error>>> cause;
};
auto error_cause(ref<str> message) -> ErrorCause { return { String::make(message), None() }; }
template<class E>
    requires rstd::Impled<rstd::mtp::rm_cvf<E>, rstd::error::Error>
auto error_cause(E&& cause) -> ErrorCause {
    return { {}, Some(Box<dyn<rstd::error::Error>>::make(rstd::forward<E>(cause))) };
}
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<fmt::Display, qcm::backend::ErrorCause> : ImplBase<qcm::backend::ErrorCause> {
    auto fmt(fmt::Formatter& f) const -> bool {
        const auto& error = this->self();
        if (error.cause.is_some())
            return f.write_fmt(fmt::Arguments::make("{}", (*error.cause).as_ref()));
        return f.write_str(error.message.as_str());
    }
};
template<>
struct Impl<error::Error, qcm::backend::ErrorCause>
    : DefaultInImpl<error::Error, qcm::backend::ErrorCause> {
    auto source() const noexcept -> Option<error::ErrorRef> {
        const auto& error = this->self();
        if (error.cause.is_some()) return Some((*error.cause).as_ref());
        return None();
    }
};
} // namespace rstd
