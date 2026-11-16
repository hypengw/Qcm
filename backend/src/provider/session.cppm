module;
#include <rstd/macro.hpp>
export module qcm.backend:provider.session;
export import rstd;
export import sqlito;
export import luato;
import :provider.lua_error;
export import :provider.generation;
export import :network.cookie_session;
import :db.database;
import :db.provider.store;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::collections::HashMap;
template<class T>
using RcWeak = alloc::rc::Weak<T>;
using namespace rstd::prelude;
using namespace rstd::literals;

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
    auto wait() -> rstd::async::coro<empty> {
        auto receiver = result_.subscribe();
        for (;;) {
            if (receiver.borrow_and_update()->is_some()) co_return empty {};
            if ((co_await receiver.changed()).is_err()) co_return empty {};
        }
    }
    auto complete(luato::Result<luato::LuaValue> result) -> void {
        if (finished()) return;
        (void)result_.send_replace(Some(rstd::move(result)));
    }
    auto cancel() -> void {
        if (finished()) return;
        complete(
            Err(provider_error_to_lua(ProviderError::NotAuth(), "qcm.inner.session_task"_str)));
    }
    auto result() const -> luato::Result<luato::LuaValue> {
        auto current = result_.borrow();
        if (current->is_none()) return Err(luato::Error::binding("session task is unfinished"_Str));
        const auto& result = **current;
        if (result.is_err()) return Err(result.as_ref().unwrap_err().clone());
        return result->clone();
    }
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
    ~SessionFlightLease() {
        if (owner_ && flight_) flight_.get()->cancel();
    }
    auto owner() const -> bool { return owner_; }
    auto flight() -> SessionFlight& { return *flight_.get(); }
};

class SessionFlights {
    HashMap<String, RcWeak<SessionFlight>> entries_;

public:
    auto acquire(ref<str> key) -> SessionFlightLease {
        if (auto entry = entries_.get(key)) {
            if (auto flight = (*entry)->upgrade(); flight.is_some() && ! (*flight)->finished())
                return SessionFlightLease(rstd::move(*flight), false);
        }
        auto flight = make_rc<SessionFlight>();
        (void)entries_.insert(String::make(key), flight.downgrade());
        return SessionFlightLease(rstd::move(flight), true);
    }
    auto cancel() -> void {
        auto entries = rstd::exchange(entries_, {});
        for (auto weak : entries.values())
            if (auto flight = weak->upgrade()) flight->get()->cancel();
    }
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
    auto bind(sqlito::ExecutorHandle db, i64 provider_id, String key) -> void {
        db_          = Some(rstd::move(db));
        provider_id_ = provider_id;
        key_         = rstd::move(key);
        binding_     = make_rc<empty>();
        saved_       = None();
    }
    auto invalidate() -> void {
        active_ = false;
        saved_  = None();
        flights_.cancel();
    }
    auto active() const -> bool { return active_; }
    auto key() const -> String { return key_.clone(); }
    auto matches(i64 provider_id, ref<str> key) const -> bool {
        return active_ && db_.is_some() && provider_id_ == provider_id && key_ == key;
    }
    auto flights() -> SessionFlights& { return flights_; }
    auto lock_save() -> rstd::async::MutexLock<empty> { return writes_.lock(); }
    auto reset_saved() -> void { saved_ = None(); }
    auto is_saved(ref<str> custom, const Option<CookieStamp>& cookies,
                  const ProviderGenerationCheck& generation) const -> bool {
        if (! active_ || db_.is_none() || saved_.is_none() || ! generation.is_current() ||
            ! saved_->generation.same_generation(generation) || saved_->custom.as_str() != custom ||
            saved_->cookies.is_some() != cookies.is_some())
            return false;
        return cookies.is_none() || saved_->cookies->matches(*cookies);
    }
    auto save(String custom, String cookie, Option<CookieStamp> cookies,
              ProviderGenerationCheck generation)
        -> rstd::async::coro<ProviderResult<SessionSaveResult>> {
        if (! active_ || ! generation.is_current()) co_return Ok(SessionSaveResult::Invalid);
        if (db_.is_none()) co_return Ok(SessionSaveResult::Staged);
        if (is_saved(custom.as_str(), cookies, generation)) co_return Ok(SessionSaveResult::Saved);
        auto provider   = provider_id_;
        auto key        = key_.clone();
        auto binding    = binding_.clone();
        auto checkpoint = Saved { custom.clone(), rstd::move(cookies), generation.clone() };
        auto saved      = rstd_co_try(co_await database_run(
            db_->clone(),
            [provider,
             key    = key.clone(),
             custom = rstd::move(custom),
             cookie = rstd::move(cookie),
             generation =
                 generation.clone()](sqlito::Connection& db) -> ProviderResult<SessionSaveResult> {
                if (! generation.is_current()) return Ok(SessionSaveResult::Invalid);
                auto saved = rstd_try(save_provider_session(db, provider, key, custom, cookie));
                return Ok(saved ? SessionSaveResult::Saved : SessionSaveResult::Invalid);
            }));
        if (! matches(provider, key.as_str()) || ! Rc<empty>::ptr_eq(binding_, binding) ||
            ! generation.is_current())
            co_return Ok(SessionSaveResult::Invalid);
        if (saved == SessionSaveResult::Saved)
            saved_ = Some(rstd::move(checkpoint));
        else
            saved_ = None();
        co_return Ok(saved);
    }
};
} // namespace qcm::backend
