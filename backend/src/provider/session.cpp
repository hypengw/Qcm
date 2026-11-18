module;
#include <rstd/macro.hpp>
module qcm.backend;
import :provider.session;
import :db.database;
import :db.provider.store;
import :provider.lua_error;
using alloc::rc::make_rc;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
auto SessionFlight::wait() -> rstd::async::coro<empty> {
    auto receiver = result_.subscribe();
    for (;;) {
        if (receiver.borrow_and_update()->is_some()) co_return empty {};
        if ((co_await receiver.changed()).is_err()) co_return empty {};
    }
}

auto SessionFlight::complete(luato::Result<luato::LuaValue> result) -> void {
    if (finished()) return;
    (void)result_.send_replace(Some(rstd::move(result)));
}

auto SessionFlight::cancel() -> void {
    if (finished()) return;
    complete(Err(provider_error_to_lua(ProviderError::NotAuth(), "qcm.inner.session_task"_str)));
}

auto SessionFlight::result() const -> luato::Result<luato::LuaValue> {
    auto current = result_.borrow();
    if (current->is_none()) return Err(luato::Error::binding("session task is unfinished"_Str));
    const auto& result = **current;
    if (result.is_err()) return Err(result.as_ref().unwrap_err().clone());
    return result->clone();
}

SessionFlightLease::~SessionFlightLease() {
    if (owner_ && flight_) flight_.get()->cancel();
}

auto SessionFlights::acquire(ref<str> key) -> SessionFlightLease {
    if (auto entry = entries_.get(key)) {
        if (auto flight = (*entry)->upgrade(); flight.is_some() && ! (*flight)->finished())
            return SessionFlightLease(rstd::move(*flight), false);
    }
    auto flight = make_rc<SessionFlight>();
    (void)entries_.insert(String::make(key), flight.downgrade());
    return SessionFlightLease(rstd::move(flight), true);
}

auto SessionFlights::cancel() -> void {
    auto entries = rstd::exchange(entries_, {});
    for (auto weak : entries.values())
        if (auto flight = weak->upgrade()) flight->get()->cancel();
}

auto ProviderSessionState::bind(sqlito::ExecutorHandle db, i64 provider_id, String key) -> void {
    db_          = Some(rstd::move(db));
    provider_id_ = provider_id;
    key_         = rstd::move(key);
    binding_     = make_rc<empty>();
    saved_       = None();
}

auto ProviderSessionState::invalidate() -> void {
    active_ = false;
    saved_  = None();
    flights_.cancel();
}

auto ProviderSessionState::is_saved(ref<str> custom, const Option<CookieStamp>& cookies,
                                    const ProviderGenerationCheck& generation) const -> bool {
    if (! active_ || db_.is_none() || saved_.is_none() || ! generation.is_current() ||
        ! saved_->generation.same_generation(generation) || saved_->custom.as_str() != custom ||
        saved_->cookies.is_some() != cookies.is_some())
        return false;
    return cookies.is_none() || saved_->cookies->matches(*cookies);
}

auto ProviderSessionState::save(String custom, String cookie, Option<CookieStamp> cookies,
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
} // namespace qcm::backend
