module qcm.backend;
import :db.error.provider;
using namespace rstd::prelude;

namespace rstd
{
auto Impl<convert::From<qcm::backend::DbWriteError>, qcm::backend::ProviderError>::from(
    qcm::backend::DbWriteError error) -> qcm::backend::ProviderError {
    return qcm::backend::ProviderError::db(rstd::move(error));
}

auto Impl<convert::From<qcm::backend::QueryError>, qcm::backend::ProviderError>::from(
    qcm::backend::QueryError error) -> qcm::backend::ProviderError {
    return qcm::backend::ProviderError::db(rstd::move(error));
}
} // namespace rstd
