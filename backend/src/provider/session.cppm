export module qcm.backend:provider.session;
export import rstd;
export import sqlito;
export import luato;
import :provider.error;
export import :provider.generation;
export import :network.cookie_session;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::collections::HashMap;
template<class T>
using RcWeak = alloc::rc::Weak<T>;
using namespace rstd::prelude;

export namespace qcm::backend
{
enum class SessionSaveResult
{
    Staged,
    Saved,
    Invalid
};

class SessionFlight {
    rstd::async::watch::Sender<Option<luato::Result<luato::LuaValue>>> result_ { None() };

public:
    auto finished() const -> bool { return result_.borrow()->is_some(); }
    auto wait() -> rstd::async::coro<empty>;
    auto complete(luato::Result<luato::LuaValue> result) -> void;
    auto cancel() -> void;
    auto result() const -> luato::Result<luato::LuaValue>;
};

class SessionFlightLease {
    Rc<SessionFlight> flight_;
    bool              owner_ {};

public:
    SessionFlightLease(Rc<SessionFlight> flight, bool owner)
        : flight_(rstd::move(flight)), owner_(owner) {}
    SessionFlightLease(const SessionFlightLease&)                    = delete;
    auto operator=(const SessionFlightLease&) -> SessionFlightLease& = delete;
    SessionFlightLease(SessionFlightLease&&) noexcept                = default;
    auto operator=(SessionFlightLease&&) -> SessionFlightLease&      = delete;
    // The coroutine frame owns this lease even before its first poll.
    ~SessionFlightLease();
    auto owner() const -> bool { return owner_; }
    auto flight() -> SessionFlight& { return *flight_.get(); }
};

class SessionFlights {
    HashMap<String, RcWeak<SessionFlight>> entries_;

public:
    auto acquire(ref<str> key) -> SessionFlightLease;
    auto cancel() -> void;
};

class ProviderSessionState {
    struct Saved {
        String                  custom;
        Option<CookieStamp>     cookies;
        ProviderGenerationCheck generation;
    };
    Option<sqlito::ExecutorHandle> db_;
    i64                            provider_id_ {};
    String                         key_;
    bool                           active_ { true };
    SessionFlights                 flights_;
    rstd::async::Mutex<empty>      writes_ { empty {} };
    Rc<empty>                      binding_ { make_rc<empty>() };
    Option<Saved>                  saved_;

public:
    auto bind(sqlito::ExecutorHandle db, i64 provider_id, String key) -> void;
    auto invalidate() -> void;
    auto active() const -> bool { return active_; }
    auto key() const -> String { return key_.clone(); }
    auto matches(i64 provider_id, ref<str> key) const -> bool {
        return active_ && db_.is_some() && provider_id_ == provider_id && key_ == key;
    }
    auto flights() -> SessionFlights& { return flights_; }
    auto lock_save() -> rstd::async::MutexLock<empty> { return writes_.lock(); }
    auto reset_saved() -> void { saved_ = None(); }
    auto is_saved(ref<str> custom, const Option<CookieStamp>& cookies,
                  const ProviderGenerationCheck& generation) const -> bool;
    auto save(String custom, String cookie, Option<CookieStamp> cookies,
              ProviderGenerationCheck generation)
        -> rstd::async::coro<ProviderResult<SessionSaveResult>>;
};
} // namespace qcm::backend
