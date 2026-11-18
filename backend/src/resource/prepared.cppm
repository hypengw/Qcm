export module qcm.backend:resource.prepared;
export import :resource.model;
export import :resource.response;
export import :resource.lifetime;
export import :network.cookie_session;
using qcm::backend::ProviderError;
using rstd::async::coro;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend::resource_cache
{
class ResourceValidity {
public:
    using Check = Box<dyn<Fn<bool()>>>;

private:
    CookieSession            session_;
    Option<PlaybackLifetime> lifetime_;
    Arc<Check>               current_;

    static auto poll_invalidated(CookieSession session, Arc<Check> current) -> coro<empty>;
    static auto wait(CookieSession session, Arc<Check> current, Option<PlaybackLifetime> lifetime)
        -> coro<empty>;

public:
    ResourceValidity(CookieSession session, Option<PlaybackLifetime> lifetime, Arc<Check> current);
    auto is_valid() const -> bool;
    auto with_lifetime(Option<PlaybackLifetime> lifetime) const -> Arc<ResourceValidity>;
    auto cancelled() const -> coro<empty>;
    auto guard(Response response) const -> Response;
};

class PreparedResource {
public:
    using Fetch = Box<dyn<Fn<coro<Result<Response, ProviderError>>(PreparedRequest, bool)>>>;
    using Check = ResourceValidity::Check;

private:
    PreparedRequest       request_;
    Arc<Fetch>            fetch_;
    Arc<ResourceValidity> validity_;

    PreparedResource(PreparedRequest request, Arc<Fetch> fetch, Arc<ResourceValidity> validity);
    static auto execute_owned(PreparedRequest request, Arc<Fetch> fetch,
                              Arc<ResourceValidity> validity, bool streaming)
        -> coro<Result<Response, ProviderError>>;
    static auto fetch_owned(PreparedRequest request, Arc<Fetch> fetch,
                            Arc<ResourceValidity> validity, lihttpto::Headers headers)
        -> coro<Result<Response, Error>>;

public:
    PreparedResource(PreparedRequest request, Fetch fetch, CookieSession session,
                     Option<PlaybackLifetime> lifetime, Check current);
    PreparedResource(PreparedRequest request, Fetch fetch, CookieSession session,
                     Option<PlaybackLifetime> lifetime, bool current);
    PreparedResource(const PreparedResource&)                    = delete;
    auto operator=(const PreparedResource&) -> PreparedResource& = delete;
    PreparedResource(PreparedResource&&)                         = default;
    auto operator=(PreparedResource&&) -> PreparedResource&      = default;
    auto clone() const -> PreparedResource;
    auto with_lifetime(Option<PlaybackLifetime> lifetime) && -> PreparedResource;
    auto request() const& -> const PreparedRequest& { return request_; }
    auto request() const&& -> const PreparedRequest& = delete;
    auto apply_headers(const lihttpto::Headers& headers) -> Result<empty, ProviderError>;
    auto replace_delivery_headers(const lihttpto::Headers& headers) -> Result<empty, ProviderError>;
    auto cache_mode() const -> CacheMode { return request_.cache_mode(); }
    auto headers() const& -> const lihttpto::Headers& { return request_.headers(); }
    auto headers() const&& -> const lihttpto::Headers& = delete;
    auto url() const& -> ref<str> { return request_.url(); }
    auto url() const&& -> ref<str> = delete;
    auto cache_url(ref<str> effective) const -> Result<String, Error>;
    auto is_valid() const -> bool;
    auto guard() const -> Arc<ResourceValidity>;
    auto cancelled() const -> coro<empty>;
    auto execute(bool streaming = true) && -> coro<Result<Response, ProviderError>>;
    auto fetch(lihttpto::Headers headers) const -> coro<Result<Response, Error>>;
};
} // namespace qcm::backend::resource_cache
