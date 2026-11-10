export module qcm.backend:network.cookie_session;
export import lifetchto;
export import rstd;
using rstd::sync::Mutex;
using rstd::sync::Weak;
using namespace rstd::prelude;
using rstd::async::coro;
using rstd::sync::Arc;

export namespace qcm::backend
{
class SessionStreamCancellation {
    lifetchto::http::BodyControl control_;

public:
    explicit SessionStreamCancellation(lifetchto::http::BodyControl control)
        : control_(rstd::move(control)) {}
    auto cancel() const -> void { control_.cancel(); }
};
class CookieSession {
    struct Status {
        bool                                             active { true };
        Vec<Weak<SessionStreamCancellation>>             streams;
    };
    struct State {
        Mutex<Status>                    status { Status {} };
        lifetchto::http::MemoryCookieJar jar;
        explicit State(lifetchto::http::MemoryCookieJar value): jar(rstd::move(value)) {}
    };
    Arc<State> state_;
    explicit CookieSession(Arc<State> state): state_(rstd::move(state)) {}

public:
    static auto make() -> lifetchto::Result<CookieSession> {
        lihttpto::CookieOptions options;
        options.allow_unverified_suffixes   = true;
        options.allow_secure_from_insecure  = true;
        options.retain_expired_replacements = true;
        options.max_age_expiry              = Some(i64(253402300799LL));
        auto jar = lifetchto::http::MemoryCookieJar::make(rstd::move(options));
        if (jar.is_err()) return Err(rstd::move(jar).unwrap_err());
        return Ok(CookieSession(Arc<State>::make(rstd::move(jar).unwrap())));
    }
    auto clone() const -> CookieSession { return CookieSession(state_.clone()); }
    auto active() const -> bool {
        auto status = state_->status.lock().unwrap();
        return status->active;
    }
    auto invalidate() const -> void {
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
    auto cancel_streams() const -> void {
        Vec<Arc<SessionStreamCancellation>> streams;
        {
            auto status = state_->status.lock().unwrap();
            for (auto& weak : status->streams)
                if (auto stream = weak.upgrade()) streams.push(rstd::move(stream));
            status->streams.clear();
        }
        for (const auto& stream : streams) stream->cancel();
    }
    auto watch(lifetchto::http::BodyControl control) const -> Arc<SessionStreamCancellation> {
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
    auto select(const lihttpto::Url& url, i64 now) const
        -> lifetchto::Result<lihttpto::CookieHeader> {
        auto status = state_->status.lock().unwrap();
        if (! status->active) return Ok(lihttpto::CookieHeader {});
        return state_->jar.select(url, now);
    }
    auto store(const lihttpto::Url& url, const lihttpto::SetCookie& cookie, i64 now) const
        -> lifetchto::Result<empty> {
        auto status = state_->status.lock().unwrap();
        if (! status->active) return Ok(empty {});
        return state_->jar.store(url, cookie, now);
    }
    auto clear() const -> lifetchto::Result<empty> {
        auto status = state_->status.lock().unwrap();
        if (! status->active) return Err(lifetchto::Error::Closed());
        return state_->jar.clear();
    }
    auto import_netscape(ref<str> input, i64 now) const -> lifetchto::Result<empty> {
        auto status = state_->status.lock().unwrap();
        if (! status->active) return Err(lifetchto::Error::Closed());
        return state_->jar.import_netscape(input, now);
    }
    auto export_netscape(i64 now) const -> lifetchto::Result<String> {
        auto status = state_->status.lock().unwrap();
        if (! status->active) return Err(lifetchto::Error::Closed());
        return state_->jar.export_netscape(now);
    }
    auto snapshot_all() const -> lifetchto::Result<Vec<lihttpto::StoredCookie>> {
        auto status = state_->status.lock().unwrap();
        if (! status->active) return Err(lifetchto::Error::Closed());
        return state_->jar.snapshot_all();
    }
    auto replace_snapshot(Vec<lihttpto::StoredCookie> cookies) const -> lifetchto::Result<empty> {
        auto status = state_->status.lock().unwrap();
        if (! status->active) return Err(lifetchto::Error::Closed());
        return state_->jar.replace_snapshot(rstd::move(cookies));
    }
    auto jar() const -> Arc<dyn<lifetchto::http::CookieJar>>;
    void request_close() const { state_->jar.request_close(); }
    auto close() const -> coro<lifetchto::Result<empty>> { return state_->jar.close(); }
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

export namespace qcm::backend
{
inline auto CookieSession::jar() const -> Arc<dyn<lifetchto::http::CookieJar>> {
    return Arc<dyn<lifetchto::http::CookieJar>>::make(clone());
}
} // namespace qcm::backend
