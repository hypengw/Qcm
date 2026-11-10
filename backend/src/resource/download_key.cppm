export module qcm.backend:resource.download_key;
export import rstd;
using namespace rstd::prelude;

export namespace qcm::backend::resource_cache
{
struct DownloadKey {
    i64         version {};
    u64         start {};
    u64         end {};
    friend auto operator==(const DownloadKey&, const DownloadKey&) -> bool = default;
};
} // namespace qcm::backend::resource_cache

namespace rstd
{
export template<>
struct Impl<hash::Hash, qcm::backend::resource_cache::DownloadKey>
    : ImplBase<qcm::backend::resource_cache::DownloadKey> {
    template<class H>
        requires Impled<H, hash::Hasher>
    void hash(H& state) const noexcept {
        hash::hash_into(this->self().version, state);
        hash::hash_into(this->self().start, state);
        hash::hash_into(this->self().end, state);
    }
};
} // namespace rstd
