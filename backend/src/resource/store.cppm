export module qcm.backend:resource.store;
export import :resource.types;
using namespace rstd::prelude;
using rstd::bytes::Bytes;
using rstd::path::PathBuf;
using rstd::sync::Arc;

namespace qcm::backend::resource_cache
{
struct Storage;
namespace store_detail
{
struct Owner;
using Job = Box<dyn<FnMut<void(Storage&)>>>;
template<class F, class Sender>
auto make_job(F function, Sender reply) -> Job {
    return Job::make(
        [function = rstd::move(function), reply = rstd::move(reply)](Storage& storage) mutable {
            auto result = function(storage);
            (void)reply.send(rstd::move(result));
        });
}
} // namespace store_detail
} // namespace qcm::backend::resource_cache

export namespace qcm::backend::resource_cache
{
class Store {
    Arc<store_detail::Owner> owner_;
    explicit Store(Arc<store_detail::Owner> owner);
    template<class T>
    static auto submit(Arc<store_detail::Owner> owner, store_detail::Job job,
                       rstd::async::oneshot::Receiver<DbResult<T>> reply) -> StorageFuture<T>;
    template<class F>
    static auto call_owned(Arc<store_detail::Owner> owner, F function)
        -> StorageFuture<typename decltype(function(rstd::declval<Storage&>()))::value_type>;
    template<class F>
    auto        call(F function) const;
    static auto shutdown_owned(Arc<store_detail::Owner> owner) -> StorageFuture<empty>;
    auto        submit_object(store_detail::Job                               job,
                              rstd::async::oneshot::Receiver<DbResult<empty>> reply) const
        -> StorageFuture<empty>;
    static auto write_object(Storage& storage, ObjectMetadata meta, slice<u8> bytes,
                             mut_ref<dyn<FnMut<bool()>>> valid) -> DbResult<empty>;

public:
    Store(Store&&) noexcept;
    ~Store();
    auto        clone() const -> Store;
    static auto open(PathBuf root) -> StorageFuture<Store>;
    auto        lookup(String key) const -> StorageFuture<Option<Version>>;
    auto        create(Metadata meta) const -> StorageFuture<Version>;
    auto        read(Version version, u64 block) const -> StorageFuture<Option<Bytes>>;
    auto        write(Version version, Blocks blocks) const -> StorageFuture<CommitOutcome>;
    auto        complete(Version version) const -> StorageFuture<bool>;
    auto        covered(Version version, Range range) const -> StorageFuture<bool>;
    auto        update(Version version, Metadata meta) const -> StorageFuture<empty>;
    auto        missing_range(Version version, Range range) const -> StorageFuture<Range>;
    auto        invalidate(i64 provider) const -> StorageFuture<empty>;
    auto        usage() const -> StorageFuture<Usage>;
    auto        checkpoint() const -> StorageFuture<empty>;
    auto        retire(Version version) const -> StorageFuture<empty>;
    auto        shutdown() const -> StorageFuture<empty>;
    auto        object(String key) const -> StorageFuture<Option<Object>>;
    template<class F>
    auto put_object(ObjectMetadata meta, Bytes bytes, F valid) const -> StorageFuture<empty> {
        auto reply = rstd::async::oneshot::channel<DbResult<empty>>();
        auto job   = store_detail::make_job(
            [meta = rstd::move(meta), bytes = rstd::move(bytes), valid = rstd::move(valid)](
                Storage& storage) mutable {
                auto check = [&valid] {
                    return static_cast<bool>(valid());
                };
                return write_object(storage,
                                    rstd::move(meta),
                                    bytes.as_slice(),
                                    dyn<FnMut<bool()>>::from_ref(check));
            },
            rstd::move(reply.template get<0>()));
        return submit_object(rstd::move(job), rstd::move(reply.template get<1>()));
    }
    auto remove_object(String key) const -> StorageFuture<empty>;
    auto update_object(ObjectMetadata meta) const -> StorageFuture<empty>;
    auto window(Version version, Range range) const -> StorageFuture<Window>;
    auto local_file(Version version, String client) const -> StorageFuture<Option<LocalFile>>;
    auto renew_file(String token, String client) const -> StorageFuture<bool>;
    auto release_file(String token, String client) const -> StorageFuture<empty>;
};
} // namespace qcm::backend::resource_cache
