export module qcm.backend:db.error.provider;
export import :provider.error;
export import :db.error.write;
export import :db.error.query;

export namespace rstd
{
template<>
struct Impl<convert::From<qcm::backend::DbWriteError>, qcm::backend::ProviderError> {
    static auto from(qcm::backend::DbWriteError error) -> qcm::backend::ProviderError;
};
template<>
struct Impl<convert::From<qcm::backend::QueryError>, qcm::backend::ProviderError> {
    static auto from(qcm::backend::QueryError error) -> qcm::backend::ProviderError;
};
} // namespace rstd
