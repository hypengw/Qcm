export module qcm.backend:db.error.process;
export import :error;
export import :db.error.write;
using namespace rstd::prelude;

export namespace rstd
{
template<>
struct Impl<convert::From<qcm::backend::DbWriteError>, qcm::backend::ProcessError> {
    static auto from(qcm::backend::DbWriteError error) -> qcm::backend::ProcessError {
        return qcm::backend::ProcessError::db(rstd::move(error));
    }
};
} // namespace rstd
