module;
#include <rstd/enum.hpp>
export module qcm.backend:db.error.write;
export import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
// clang-format off
[[pmacro::invoke(
    "rstd::Enum",
    [[derive(Debug, Error)]] enum DbWriteError {
        [[error("{error}")]]
        Storage([[from]] error : sqlito::Error),
        [[error("{error}")]]
        Binding([[from]] error : liorm::BindError<sqlito::Error>),
        [[error("{error}")]]
        WritePlan([[from]] error : liorm::WriteError),
        [[error("{error}")]]
        Query([[from]] error : liorm::query::Error),
        [[error("{message}")]]
        Invalid(message : String),
    })]];
// clang-format on
template<class T>
using DbWriteResult = Result<T, DbWriteError>;
static_assert(! rstd::mtp::copy<DbWriteError>);
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<convert::From<liorm::sqlito::ActiveWriteError>, qcm::backend::DbWriteError> {
    static auto from(liorm::sqlito::ActiveWriteError error) -> qcm::backend::DbWriteError {
        using qcm::backend::DbWriteError;
        RSTD_MATCH(error) {
            RSTD_CASE(Planning, cause) { return DbWriteError::WritePlan(rstd::move(cause)); }
            RSTD_CASE(Storage, cause) { return DbWriteError::Storage(rstd::move(cause)); }
            RSTD_CASE(Binding, cause) { return DbWriteError::Binding(rstd::move(cause)); }
            RSTD_CASE(NotFound) {
                return DbWriteError::Invalid(String::make("write target not found"_str));
            }
            RSTD_CASE(NotInserted) {
                return DbWriteError::Invalid(String::make("row was not inserted"_str));
            }
            RSTD_CASE(NotUpdated) {
                return DbWriteError::Invalid(String::make("row was not updated"_str));
            }
        }
    }
};
} // namespace rstd
