export module qcm.backend:resource.service;
export import :resource.prepared;
export import :resource.file_delivery;
using namespace rstd::prelude;
using lihttpto::Headers;
using rstd::tuple;
using rstd::async::AcquireError;
using rstd::async::coro;
using rstd::async::OwnedSemaphorePermit;
using rstd::bytes::Bytes;
using rstd::sync::Arc;

namespace qcm::backend::resource_cache
{
struct Range;
struct Version;
struct Metadata;
class Store;
} // namespace qcm::backend::resource_cache

export namespace qcm::backend::resource_cache
{
struct ResourceRequest {
    i64                   provider;
    ResourceKind          kind;
    String                scope;
    Arc<PreparedResource> prepared;
    Headers               headers;
    auto                  clone() const -> ResourceRequest;
};

class ResourceCache {
    template<typename T>
    using ServiceResult = Result<T, Error>;
    static inline constexpr u64   FETCH_BYTES { 4 * 1024 * 1024 };
    static inline constexpr usize OUTPUT_BYTES { 64 * 1024 };
    struct ReaderPosition;
    class Progress;
    struct DownloadState;
    struct Download;
    struct BlockOutput;
    struct State;

    struct Reader;
    struct UpstreamBody;
    struct ObjectBody;
    class OpenKey {
        friend class ResourceCache;
        OpenKey() = default;

    public:
        OpenKey(OpenKey&&) = default;
    };
    Box<State> state_;

    auto        owner() const -> Arc<ResourceCache>;
    auto        storage() -> Store&;
    static auto acquire_error(AcquireError error) -> Error;
    auto        wait_stopped() -> coro<empty>;
    auto        respond(ResourceRequest request) -> coro<ServiceResult<ResourceDelivery>>;
    auto        respond_object(ResourceRequest request) -> coro<ServiceResult<ResourceDelivery>>;
    auto        open_lock(ref<str> key) -> coro<Arc<rstd::async::Mutex<empty>>>;
    auto execute(Arc<PreparedResource> prepared, Headers headers) -> coro<ServiceResult<Response>>;
    auto fetch(Arc<PreparedResource> prepared, Headers headers) -> coro<ServiceResult<Response>>;
    auto block(Version version, Arc<PreparedResource> prepared, Headers base, u64 index,
               Option<Arc<Download>> initial, Arc<ReaderPosition> position, u64 window)
        -> coro<ServiceResult<BlockOutput>>;
    auto download(Version version, Arc<PreparedResource> prepared, Headers base, Range range,
                  Option<Response> response, Arc<ReaderPosition> position)
        -> coro<ServiceResult<Arc<Download>>>;
    auto fetch_blocks(Arc<Download> download, Version version, Arc<PreparedResource> prepared,
                      Headers headers, Range range, Option<Response> response)
        -> coro<ServiceResult<empty>>;
    auto flush_batch(Arc<Download> download, const Version& version, Vec<tuple<u64, Bytes>>& batch)
        -> coro<bool>;
    static auto run_download(Arc<ResourceCache> cache, Arc<Download> download, Version version,
                             Arc<PreparedResource> prepared, Headers base, Range range,
                             Option<Response> response) -> coro<empty>;
    static auto wait_download_cancelled(Arc<Download> download) -> coro<empty>;
    static auto download_closed(Arc<ResourceCache> cache, Arc<PreparedResource> prepared,
                                Arc<Download> download) -> coro<empty>;
    auto        file_client_for(const Headers& headers) const -> Option<String>;
    auto        authenticated_file_client(const Headers& headers) const -> Option<String>;
    static auto request_headers(const Headers& prepared, const Headers& client) -> Headers;
    static auto downstream_request(const Headers& base, const Headers& client) -> Headers;
    static auto request_identity(const ResourceRequest& request, const Headers& base)
        -> ServiceResult<String>;
    static auto target_identity(const PreparedResource& prepared, const Response& response)
        -> ServiceResult<String>;
    static auto response_geometry(const Response& response)
        -> ServiceResult<Option<tuple<u64, Range>>>;
    static auto identity_encoding(ref<str> encoding) -> bool;
    static auto client_range(const Headers& headers, const Metadata& meta) -> Option<Range>;
    static auto range_requested(const Headers& headers, const Metadata& meta) -> bool;
    static auto empty_response(i32 status, Headers headers = {}) -> Response;
    auto passthrough(Response response, OwnedSemaphorePermit reader, Arc<PreparedResource> prepared,
                     Option<OwnedSemaphorePermit> buffer   = {},
                     Option<OwnedSemaphorePermit> transfer = {}, Vec<Bytes> prefix = {})
        -> ResourceDelivery;
    static auto ensure_validation(const PreparedResource& prepared, const Metadata& old,
                                  const Response& response) -> ServiceResult<empty>;
    static auto resource_generation() -> ServiceResult<String>;

public:
    // Arc constructs in place; only open can supply its construction key.
    explicit ResourceCache(OpenKey, Box<State> state);
    ~ResourceCache();
    static auto open(rstd::path::PathBuf root) -> coro<ServiceResult<Arc<ResourceCache>>>;
    auto        usage() -> coro<ServiceResult<Usage>>;
    auto        invalidate(i64 provider) -> coro<ServiceResult<empty>>;
    auto        shutdown() -> coro<ServiceResult<empty>>;
    auto        delivery(ResourceRequest request) -> coro<ServiceResult<ResourceDelivery>>;
    auto        forward(Response response) -> coro<ServiceResult<ResourceDelivery>>;
    auto        object_response(ResourceRequest request) -> coro<ServiceResult<ResourceDelivery>>;
    auto        authorize_file_client(ref<str> key) -> ServiceResult<empty>;
    static auto file_lease_response(Option<Arc<ResourceCache>> cache, ref<str> method,
                                    ref<str> path, const Headers& headers)
        -> coro<Option<Response>>;
};
} // namespace qcm::backend::resource_cache
