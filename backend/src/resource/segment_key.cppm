export module qcm.backend:resource.segment_key;
export import rstd;
using namespace rstd::prelude;

export namespace qcm::backend::resource_cache
{
struct SegmentKey {
    i64         version {};
    u64         number {};
    friend auto operator==(const SegmentKey&, const SegmentKey&) -> bool = default;
};
} // namespace qcm::backend::resource_cache

namespace rstd
{
export template<>
struct Impl<hash::Hash, qcm::backend::resource_cache::SegmentKey>
    : ImplBase<qcm::backend::resource_cache::SegmentKey> {
    template<class H>
        requires Impled<H, hash::Hasher>
    void hash(H& state) const noexcept {
        hash::hash_into(this->self().version, state);
        hash::hash_into(this->self().number, state);
    }
};
} // namespace rstd
