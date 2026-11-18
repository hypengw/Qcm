export module qcm.backend:resource.model;
export import rstd;
export import lihttpto;
export import :provider.error;
using rstd::path::PathBuf;
using rstd::time::Duration;
using namespace rstd::prelude;

export namespace qcm::backend
{
auto resource_header_allowed(const lihttpto::HeaderName& name) -> bool;
} // namespace qcm::backend

export namespace qcm::backend::resource_cache
{
enum class CacheMode
{
    ByteRange,
    WholeObject,
    Document
};
enum class ResourceKind
{
    Image = 0,
    Audio = 1
};
struct Usage {
    u64 image {}, audio {}, metadata {};
};
struct LocalFile {
    PathBuf path;
    String  token;
    u64     lifetime {};
    String  generation;
};
class PreparedRequest {
    lihttpto::Url     url_;
    String            cache_url_;
    lihttpto::Headers headers_;
    lihttpto::Headers delivery_headers_;
    Option<Duration>  timeout_;
    CacheMode         mode_ { CacheMode::ByteRange };
    Vec<String>       transient_query_;

    PreparedRequest(lihttpto::Url url, String cache_url, lihttpto::Headers headers,
                    lihttpto::Headers delivery, Option<Duration> timeout, CacheMode mode,
                    Vec<String> transient);
    static auto clear_conditions(lihttpto::Headers& headers) -> void;
    static auto normalized_url(const lihttpto::Url& url, const Vec<String>& transient) -> String;

public:
    static auto make(ref<str> method, ref<str> url, lihttpto::Headers headers, bool has_body,
                     Option<Duration> timeout = None()) -> ProviderResult<PreparedRequest>;
    auto        with_cache_mode(CacheMode mode) && -> PreparedRequest {
        mode_ = mode;
        return rstd::move(*this);
    }
    auto with_transient_query(Vec<String> names) && -> ProviderResult<PreparedRequest>;
    auto url() const -> ref<str> { return url_.as_ref(); }
    auto request_parts() const -> lihttpto::RequestParts;
    auto cache_url() const -> ref<str>;
    auto cache_url(ref<str> effective) const -> ProviderResult<String>;
    auto cache_mode() const -> CacheMode { return mode_; }
    auto headers() const -> const lihttpto::Headers& { return headers_; }
    auto delivery_headers() const -> const lihttpto::Headers& { return delivery_headers_; }
    auto timeout() const -> Option<Duration> { return timeout_; }
    auto transient_query() const -> const Vec<String>& { return transient_query_; }
    auto apply_headers(const lihttpto::Headers& downstream) -> ProviderResult<empty>;
    auto replace_delivery_headers(const lihttpto::Headers& downstream) -> ProviderResult<empty>;
    auto extend_delivery_headers(const lihttpto::Headers& headers) -> void;
    auto clone() const -> PreparedRequest;
};
} // namespace qcm::backend::resource_cache
