export module qcm.backend:resource.body;
export import :resource.error;
using rstd::async::coro;
using rstd::bytes::Bytes;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend::resource_cache
{
struct ResourceBodySource {
    template<class Self, class = void>
    struct Api {
        using Trait = ResourceBodySource;
        auto next() const -> coro<Result<Option<Bytes>, Error>> {
            return rstd::trait_call<0>(this);
        }
        auto close() const -> coro<empty> { return rstd::trait_call<1>(this); }
        void cancel() const { rstd::trait_call<2>(this); }
    };
    template<class T>
    using Funcs = rstd::TraitFuncs<&T::next, &T::close, &T::cancel>;
};
} // namespace qcm::backend::resource_cache

namespace qcm::backend::resource_cache
{
template<class T>
struct ResourceBodyOwner {
    Arc<T> owner;
};

template<class Stream, class Owner>
struct ResourceStreamBody {
    Arc<Owner>     owner;
    Option<Stream> stream;

    ResourceStreamBody(Stream stream, Arc<Owner> owner)
        : owner(rstd::move(owner)), stream(Some(rstd::move(stream))) {}
    ~ResourceStreamBody() { cancel(); }
    auto next() -> coro<Result<Option<Bytes>, Error>> {
        if (stream.is_none()) co_return Ok(None());
        auto item = co_await rstd::stream::next(*stream);
        if (item.is_none()) co_return Ok(None());
        auto result = rstd::move(item).unwrap();
        if (result.is_err()) co_return Err(rstd::move(result).unwrap_err());
        co_return Ok(Some(rstd::move(result).unwrap()));
    }
    void cancel() {
        // Unfold retains its pending future after the next waiter is dropped.
        stream = None();
        owner->cancel();
    }
    auto close() -> coro<empty> {
        stream = None();
        co_await owner->close();
        co_return empty {};
    }
};

struct ResourceBodyControl;
} // namespace qcm::backend::resource_cache

export namespace rstd
{
template<class T>
struct Impl<qcm::backend::resource_cache::ResourceBodySource,
            qcm::backend::resource_cache::ResourceBodyOwner<T>>
    : ImplBase<qcm::backend::resource_cache::ResourceBodyOwner<T>> {
    auto next() const -> coro<Result<Option<Bytes>, qcm::backend::resource_cache::Error>> {
        return this->self().owner->next();
    }
    auto close() const -> coro<empty> { return this->self().owner->close(); }
    void cancel() const { this->self().owner->cancel(); }
};
} // namespace rstd

export namespace qcm::backend::resource_cache
{
class ResourceBody {
    Option<Arc<ResourceBodyControl>> source_;

    auto owner() const -> Option<Arc<ResourceBodyControl>>;

public:
    ResourceBody() noexcept;
    explicit ResourceBody(Box<dyn<ResourceBodySource>> source);
    ResourceBody(const ResourceBody&)                    = delete;
    auto operator=(const ResourceBody&) -> ResourceBody& = delete;
    ResourceBody(ResourceBody&& other) noexcept;
    auto operator=(ResourceBody&& other) noexcept -> ResourceBody&;
    ~ResourceBody();
    static auto from_bytes(Bytes bytes) -> ResourceBody;

    // Capture ownership before the coroutine's first poll, not through this.
    auto next() -> coro<Result<Option<Bytes>, Error>>;
    auto collect() -> coro<Result<Bytes, Error>>;
    auto close() -> coro<empty>;
    void cancel();
};

template<class T>
auto resource_body(Arc<T> owner) -> ResourceBody {
    return ResourceBody(
        Box<dyn<ResourceBodySource>>::make(ResourceBodyOwner<T> { rstd::move(owner) }));
}

template<rstd::stream::Source Stream, class Owner>
    requires rstd::mtp::same_as<rstd::stream::stream_item_t<Stream>, Result<Bytes, Error>>
auto resource_body(Stream stream, Arc<Owner> owner) -> ResourceBody {
    return resource_body(
        Arc<ResourceStreamBody<Stream, Owner>>::make(rstd::move(stream), rstd::move(owner)));
}
} // namespace qcm::backend::resource_cache
