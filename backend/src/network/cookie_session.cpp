module qcm.backend;
import :network.cookie_session;
using rstd::sync::Mutex;
using rstd::sync::Weak;
using namespace rstd::prelude;
using rstd::async::coro;
using rstd::sync::Arc;

namespace qcm::backend
{
struct CookieSession::Status {
    bool                                 active { true };
    Vec<Weak<SessionStreamCancellation>> streams;
};
struct CookieSession::State {
    Mutex<Status>                    status { Status {} };
    lifetchto::http::MemoryCookieJar jar;
    explicit State(lifetchto::http::MemoryCookieJar value): jar(rstd::move(value)) {}
};

CookieSession::CookieSession(CookieSession&&) noexcept                    = default;
auto CookieSession::operator=(CookieSession&&) noexcept -> CookieSession& = default;
CookieSession::~CookieSession()                                           = default;

SessionStreamCancellation::SessionStreamCancellation(lifetchto::http::BodyControl control)
    : control_(rstd::move(control)) {}

auto SessionStreamCancellation::cancel() const -> void { control_.cancel(); }

CookieSession::CookieSession(Arc<State> state): state_(rstd::move(state)) {}

auto CookieSession::make() -> lifetchto::Result<CookieSession> {
    lihttpto::CookieOptions options;
    options.allow_unverified_suffixes   = true;
    options.allow_secure_from_insecure  = true;
    options.retain_expired_replacements = true;
    options.max_age_expiry              = Some(i64(253402300799LL));
    auto jar = lifetchto::http::MemoryCookieJar::make(rstd::move(options));
    if (jar.is_err()) return Err(rstd::move(jar).unwrap_err());
    return Ok(CookieSession(Arc<State>::make(rstd::move(jar).unwrap())));
}

auto CookieSession::clone() const -> CookieSession { return CookieSession(state_.clone()); }

auto CookieSession::same_session(const CookieSession& other) const -> bool {
    return Arc<State>::ptr_eq(state_, other.state_);
}

auto CookieSession::active() const -> bool {
    auto status = state_->status.lock().unwrap();
    return status->active;
}

auto CookieSession::invalidate() const -> void {
    Vec<Arc<SessionStreamCancellation>> streams;
    {
        auto status    = state_->status.lock().unwrap();
        status->active = false;
        for (auto& weak : status->streams)
            if (auto stream = weak.upgrade()) streams.push(rstd::move(stream));
        status->streams.clear();
    }
    for (const auto& stream : streams) stream->cancel();
}

auto CookieSession::cancel_streams() const -> void {
    Vec<Arc<SessionStreamCancellation>> streams;
    {
        auto status = state_->status.lock().unwrap();
        for (auto& weak : status->streams)
            if (auto stream = weak.upgrade()) streams.push(rstd::move(stream));
        status->streams.clear();
    }
    for (const auto& stream : streams) stream->cancel();
}

auto CookieSession::watch(lifetchto::http::BodyControl control) const
    -> Arc<SessionStreamCancellation> {
    auto stream = Arc<SessionStreamCancellation>::make(rstd::move(control));
    bool active;
    {
        auto status = state_->status.lock().unwrap();
        active      = status->active;
        if (active) {
            status->streams.retain([](const auto& weak) {
                return weak.strong_count() != usize();
            });
            status->streams.push(stream.downgrade());
        }
    }
    if (! active) stream->cancel();
    return stream;
}

auto CookieSession::select(const lihttpto::Url& url, i64 now) const
    -> lifetchto::Result<lihttpto::CookieHeader> {
    auto status = state_->status.lock().unwrap();
    if (! status->active) return Ok(lihttpto::CookieHeader {});
    return state_->jar.select(url, now);
}

auto CookieSession::store(const lihttpto::Url& url, const lihttpto::SetCookie& cookie,
                          i64 now) const -> lifetchto::Result<empty> {
    auto status = state_->status.lock().unwrap();
    if (! status->active) return Ok(empty {});
    return state_->jar.store(url, cookie, now);
}

auto CookieSession::clear() const -> lifetchto::Result<empty> {
    auto status = state_->status.lock().unwrap();
    if (! status->active) return Err(lifetchto::Error::Closed());
    return state_->jar.clear();
}

auto CookieSession::import_netscape(ref<str> input, i64 now) const -> lifetchto::Result<empty> {
    auto status = state_->status.lock().unwrap();
    if (! status->active) return Err(lifetchto::Error::Closed());
    return state_->jar.import_netscape(input, now);
}

auto CookieSession::export_netscape(i64 now) const -> lifetchto::Result<String> {
    auto status = state_->status.lock().unwrap();
    if (! status->active) return Err(lifetchto::Error::Closed());
    return state_->jar.export_netscape(now);
}

auto CookieSession::snapshot_all() const -> lifetchto::Result<Vec<lihttpto::StoredCookie>> {
    auto status = state_->status.lock().unwrap();
    if (! status->active) return Err(lifetchto::Error::Closed());
    return state_->jar.snapshot_all();
}

auto CookieSession::revision() const -> lifetchto::Result<Option<u64>> {
    auto status = state_->status.lock().unwrap();
    if (! status->active) return Err(lifetchto::Error::Closed());
    return state_->jar.revision();
}

auto CookieSession::with_snapshot(mut_ref<dyn<lifetchto::http::CookieSnapshotVisitor>> visitor)
    const -> lifetchto::Result<empty> {
    auto status = state_->status.lock().unwrap();
    if (! status->active) return Err(lifetchto::Error::Closed());
    return state_->jar.with_snapshot(visitor);
}

auto CookieSession::replace_snapshot(Vec<lihttpto::StoredCookie> cookies) const
    -> lifetchto::Result<empty> {
    auto status = state_->status.lock().unwrap();
    if (! status->active) return Err(lifetchto::Error::Closed());
    return state_->jar.replace_snapshot(rstd::move(cookies));
}

auto CookieSession::close() const -> coro<lifetchto::Result<empty>> { return state_->jar.close(); }

CookieStamp::CookieStamp(CookieSession session, Option<u64> revision)
    : session_(rstd::move(session)), revision_(revision) {}

auto CookieStamp::clone() const -> CookieStamp { return CookieStamp(session_.clone(), revision_); }

auto CookieStamp::matches(const CookieStamp& other) const -> bool {
    return revision_.is_some() && other.revision_.is_some() && revision_ == other.revision_ &&
           session_.same_session(other.session_);
}

auto CookieSession::stamp() const -> lifetchto::Result<CookieStamp> {
    auto value = revision();
    if (value.is_err()) return Err(rstd::move(value).unwrap_err());
    return Ok(CookieStamp(clone(), *value));
}

auto CookieSession::jar() const -> Arc<dyn<lifetchto::http::CookieJar>> {
    return Arc<dyn<lifetchto::http::CookieJar>>::make(clone());
}
void CookieSession::request_close() const { state_->jar.request_close(); }
} // namespace qcm::backend
