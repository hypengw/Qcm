export module qcm.backend:network.cookie_session;
export import lifetchto;
export import rstd;
using namespace rstd::prelude;
using rstd::async::coro;
using rstd::sync::Arc;

export namespace qcm::backend
{
class CookieStamp;
class SessionStreamCancellation {
    lifetchto::http::BodyControl control_;

public:
    explicit SessionStreamCancellation(lifetchto::http::BodyControl control);
    auto cancel() const -> void;
};
class CookieSession {
    struct Status;
    struct State;
    Arc<State> state_;
    explicit CookieSession(Arc<State> state);

public:
    CookieSession(const CookieSession&)                    = delete;
    auto operator=(const CookieSession&) -> CookieSession& = delete;
    CookieSession(CookieSession&&) noexcept;
    auto operator=(CookieSession&&) noexcept -> CookieSession&;
    ~CookieSession();
    static auto make() -> lifetchto::Result<CookieSession>;
    auto        clone() const -> CookieSession;
    auto        same_session(const CookieSession& other) const -> bool;
    auto        active() const -> bool;
    auto        invalidate() const -> void;
    auto        cancel_streams() const -> void;
    auto        watch(lifetchto::http::BodyControl control) const -> Arc<SessionStreamCancellation>;
    auto        select(const lihttpto::Url& url, i64 now) const
        -> lifetchto::Result<lihttpto::CookieHeader>;
    auto store(const lihttpto::Url& url, const lihttpto::SetCookie& cookie, i64 now) const
        -> lifetchto::Result<empty>;
    auto clear() const -> lifetchto::Result<empty>;
    auto import_netscape(ref<str> input, i64 now) const -> lifetchto::Result<empty>;
    auto export_netscape(i64 now) const -> lifetchto::Result<String>;
    auto snapshot_all() const -> lifetchto::Result<Vec<lihttpto::StoredCookie>>;
    auto revision() const -> lifetchto::Result<Option<u64>>;
    auto with_snapshot(mut_ref<dyn<lifetchto::http::CookieSnapshotVisitor>> visitor) const
        -> lifetchto::Result<empty>;
    auto stamp() const -> lifetchto::Result<CookieStamp>;
    auto replace_snapshot(Vec<lihttpto::StoredCookie> cookies) const -> lifetchto::Result<empty>;
    auto jar() const -> Arc<dyn<lifetchto::http::CookieJar>>;
    void request_close() const;
    auto close() const -> coro<lifetchto::Result<empty>>;
};

class CookieStamp {
    CookieSession session_;
    Option<u64>   revision_;

public:
    CookieStamp(CookieSession session, Option<u64> revision);
    auto clone() const -> CookieStamp;
    auto matches(const CookieStamp& other) const -> bool;
};
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<lifetchto::http::CookieJar, qcm::backend::CookieSession>
    : ImplBase<qcm::backend::CookieSession> {
    auto select(const lihttpto::Url& url, i64 now) const { return this->self().select(url, now); }
    auto store(const lihttpto::Url& url, const lihttpto::SetCookie& value, i64 now) const {
        return this->self().store(url, value, now);
    }
    void request_close() const { this->self().request_close(); }
    auto close() const { return this->self().close(); }
};
} // namespace rstd
