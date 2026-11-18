export module qcm.backend:network.client;
export import lifetchto;
export import rstd;
export import rstd.serde;
export import :network.cookie_session;
using rstd::async::RuntimeHandle;
using rstd::sync::Mutex;
using rstd::time::Duration;
using namespace rstd::prelude;
using rstd::async::coro;
using rstd::bytes::Bytes;
using rstd::collections::BTreeMap;
using rstd::sync::Arc;

export namespace qcm::backend
{
namespace network
{
struct ProxyEntry {
    String   id, name, url;
    Vec<i64> provider_ids;
    String   display_url;
};
template<rstd::serde::Direction Mode, class... Extra>
using ProxyEntryFields = rstd::serde::RecordDefinition<
    ProxyEntry, "ProxyEntry", Mode, rstd::serde::UnknownFieldPolicy::Ignore,
    rstd::serde::Member<&ProxyEntry::id, "id", rstd::serde::FieldPolicy::Defaulted>,
    rstd::serde::Member<&ProxyEntry::name, "name">, rstd::serde::Member<&ProxyEntry::url, "url">,
    rstd::serde::Member<&ProxyEntry::provider_ids, "provider_ids",
                        rstd::serde::FieldPolicy::Defaulted>,
    Extra...>;
struct ProxyEntryDefinition
    : ProxyEntryFields<rstd::serde::Direction::Both,
                       rstd::serde::Member<&ProxyEntry::display_url, "display_url">> {
    template<class D>
    static auto deserialize(D& source) -> Result<ProxyEntry, typename D::error_type> {
        // display_url is compiled from url, never accepted from settings input.
        return ProxyEntryFields<rstd::serde::Direction::Deserialize>::deserialize(source);
    }
};
constexpr auto rstd_serde_definition(ProxyEntry*) { return ProxyEntryDefinition {}; }
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", ignore_unknown)]]
NetworkSettings {
    u64             revision {};
    Vec<ProxyEntry> proxies;
};
class ClientPolicy {
    Option<String>                key_;
    lifetchto::http::ProxyOptions proxy_;

public:
    ClientPolicy() = default;
    ClientPolicy(String key, lifetchto::http::ProxyOptions proxy)
        : key_(Some(rstd::move(key))), proxy_(rstd::move(proxy)) {}
    auto clone() const -> ClientPolicy {
        if (key_.is_none()) return ClientPolicy {};
        return ClientPolicy { key_->clone(), proxy_.clone() };
    }
    auto same_as(const ClientPolicy& other) const -> bool { return key_ == other.key_; }
    auto proxy() const -> lifetchto::http::ProxyOptions { return proxy_.clone(); }
};
class CompiledSettings {
    BTreeMap<i64, ClientPolicy> routes_;
    friend auto compile(NetworkSettings) -> Result<CompiledSettings, lifetchto::Error>;
    friend auto publish(CompiledSettings) -> void;
    CompiledSettings(NetworkSettings value, BTreeMap<i64, ClientPolicy> routes)
        : routes_(rstd::move(routes)), settings(rstd::move(value)) {}

public:
    NetworkSettings settings;
};
auto compile(NetworkSettings settings) -> Result<CompiledSettings, lifetchto::Error>;
auto publish(CompiledSettings compiled) -> void;
auto policy(Option<i64> provider) -> ClientPolicy;
} // namespace network
} // namespace qcm::backend

export namespace qcm::backend
{
using NetworkProxy = lifetchto::http::ProxyEndpoint;
// Parses a proxy origin such as "http://127.0.0.1:8080" or "socks5://host:1080".
auto parse_proxy(ref<str> url) -> Result<NetworkProxy, lifetchto::Error>;
// Provider-facing HTTP client. The engine owns the async runtime resources and
// the native backend; callers keep one client per runtime.
struct HttpResponse {
    u16               status {};
    Vec<u8>           body;
    lihttpto::Headers headers;
    String            url;
};
class HttpResponseStream {
    lifetchto::http::Client                               client_;
    lifetchto::Transfer<lifetchto::http::ResponseMessage> transfer_;
    lifetchto::http::ResponseMessage                      message_;
    String                                                url_;

public:
    HttpResponseStream(lifetchto::http::Client                               client,
                       lifetchto::Transfer<lifetchto::http::ResponseMessage> transfer,
                       lifetchto::http::ResponseMessage message, String url)
        : client_(rstd::move(client)),
          transfer_(rstd::move(transfer)),
          message_(rstd::move(message)),
          url_(rstd::move(url)) {}
    HttpResponseStream(HttpResponseStream&&) noexcept                    = default;
    HttpResponseStream(const HttpResponseStream&)                        = delete;
    auto operator=(HttpResponseStream&&) noexcept -> HttpResponseStream& = default;

    auto head() const -> const lihttpto::ResponseHead& { return message_.head; }
    auto url() const -> ref<str> { return url_.as_str(); }
    auto control() const -> lifetchto::http::BodyControl { return message_.body.completion(); }
    auto next() -> coro<Result<Option<Bytes>, lifetchto::Error>>;
    auto cancel() -> void;
    auto read(usize limit) -> coro<Result<Vec<u8>, lifetchto::Error>>;
    auto close() -> coro<empty>;
};
class HttpClient {
    struct Clients {
        network::ClientPolicy   policy;
        lifetchto::http::Client normal, stream;
        Option<i64>             scope;
        bool                    closed {};
        Clients(lifetchto::http::Client normal, lifetchto::http::Client stream)
            : normal(rstd::move(normal)), stream(rstd::move(stream)) {}
    };
    Arc<lifetchto::Engine>    engine_;
    Arc<Mutex<Clients>>       clients_;
    Arc<Mutex<CookieSession>> cookies_;
    HttpClient(Arc<lifetchto::Engine> engine, lifetchto::http::Client normal,
               lifetchto::http::Client stream, CookieSession jar);
    static auto make_client(const lifetchto::Engine& engine, const network::ClientPolicy& policy,
                            bool streaming) -> Result<lifetchto::http::Client, lifetchto::Error>;
    auto        client(bool streaming) -> Result<lifetchto::http::Client, lifetchto::Error>;

public:
    static auto make(RuntimeHandle runtime) -> Result<HttpClient, lifetchto::Error>;
    auto        set_scope(Option<i64> scope) -> void;
    auto        cookie_session() const -> CookieSession;
    auto        reset_session() -> lifetchto::Result<empty>;
    auto        invalidate_session() -> void;
    auto        get(ref<str> url, usize limit) -> coro<Result<String, lifetchto::Error>>;
    auto open(ref<str> method_text, ref<str> url, lihttpto::Headers headers, Option<Vec<u8>> body,
              Option<Duration> timeout = None())
        -> coro<Result<HttpResponseStream, lifetchto::Error>>;
    auto open_session(ref<str> method_text, ref<str> url, lihttpto::Headers headers,
                      Option<Vec<u8>> body, Option<Duration> timeout, CookieSession session,
                      bool streaming = false) -> coro<Result<HttpResponseStream, lifetchto::Error>>;
    auto open_session(lihttpto::RequestParts parts, Option<Duration> timeout, CookieSession session,
                      bool streaming = false) -> coro<Result<HttpResponseStream, lifetchto::Error>>;

private:
    static auto open_request(lifetchto::http::Client selected, lihttpto::RequestParts parts,
                             lifetchto::http::UploadPlan upload, Option<Duration> timeout,
                             CookieSession session, bool streaming)
        -> coro<Result<HttpResponseStream, lifetchto::Error>>;

    static auto read_response(HttpResponseStream stream, usize limit)
        -> coro<Result<HttpResponse, lifetchto::Error>>;

public:
    auto request(ref<str> method_text, ref<str> url, lihttpto::Headers headers,
                 Option<Vec<u8>> body, usize limit, Option<Duration> timeout = None())
        -> coro<Result<HttpResponse, lifetchto::Error>>;
    auto request_session(ref<str> method_text, ref<str> url, lihttpto::Headers headers,
                         Option<Vec<u8>> body, usize limit, Option<Duration> timeout,
                         CookieSession session) -> coro<Result<HttpResponse, lifetchto::Error>>;
    auto request_session(lihttpto::RequestParts parts, usize limit, Option<Duration> timeout,
                         CookieSession session) -> coro<Result<HttpResponse, lifetchto::Error>>;
    auto cookie(ref<str> url, ref<str> name) -> Option<String>;
    auto set_cookie(ref<str> url, ref<str> cookie) -> bool;
    auto clear_cookies() -> void;
    static auto apply_cookies(const CookieSession& jar, const lihttpto::Url& url,
                              lihttpto::Headers& headers) -> void;
    auto        load_cookies(ref<str> data) -> bool;
    auto        save_cookies() -> Result<String, lifetchto::Error>;
    auto        close() -> coro<empty>;
};
} // namespace qcm::backend
