module;
#include <rstd/macro.hpp>
export module qcm.backend.system.dbus;
import qcm.backend;
import rstd;
import dbusto;

using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
inline constexpr auto daemon_bus_name    = "io.github.hypengw.Qcm.Daemon"_str;
inline constexpr auto daemon_object_path = "/io/github/hypengw/Qcm/Daemon"_str;
inline constexpr auto daemon_interface   = "io.github.hypengw.Qcm.Daemon"_str;

struct DaemonHost {
    template<class Self, class = void>
    struct Api {
        using Trait = DaemonHost;
        auto open_ui(String token) const -> coro<Result<empty, String>> {
            return rstd::trait_call<0>(this, rstd::move(token));
        }
        auto control(p_qcm::p_msg::M_PlayerControlReq request) const
            -> coro<Result<empty, String>> {
            return rstd::trait_call<1>(this, rstd::move(request));
        }
    };
    template<class T>
    using Funcs = rstd::TraitFuncs<&T::open_ui, &T::control>;
};
} // namespace qcm::backend

namespace qcm::backend
{
struct DaemonEndpoint {
    String                           version;
    u16                              port {};
    Option<Rc<dyn<DaemonHost>>>      host;
    Option<String>                   pending_token;
    bool                             stopped {};
    rstd::async::watch::Sender<bool> stopped_notice { false };

    explicit DaemonEndpoint(String version): version(rstd::move(version)) {}
    static auto failed(String reason) -> dbusto::Error {
        return dbusto::Error::Remote(String::make("org.freedesktop.DBus.Error.Failed"_str),
                                     rstd::move(reason));
    }
    static auto stopping() -> dbusto::Error {
        return failed(String::make("daemon bus is stopping"_str));
    }
    auto open_ui(String token) -> coro<dbusto::Result<dbusto::Body>> {
        if (stopped) co_return Err(stopping());
        if (host.is_none()) {
            if (pending_token.is_none() || ! token.is_empty())
                pending_token = Some(rstd::move(token));
            co_return Ok(dbusto::Body::make());
        }
        auto active = host->clone();
        auto result = co_await active->open_ui(rstd::move(token));
        if (result.is_err()) co_return Err(failed(rstd::move(result).unwrap_err()));
        co_return Ok(dbusto::Body::make());
    }
    auto action(p_qcm::p_msg::E_PlayerAction action) -> coro<dbusto::Result<dbusto::Body>> {
        if (stopped) co_return Err(stopping());
        if (host.is_none())
            co_return Err(dbusto::remote_error("io.github.hypengw.Qcm.Daemon.Error.NotReady"_str,
                                               "daemon backend is not ready"_str));
        auto                             active = host->clone();
        p_qcm::p_msg::M_PlayerControlReq request;
        request.f_action = i32(static_cast<int>(action));
        auto result      = co_await active->control(rstd::move(request));
        if (result.is_err()) co_return Err(failed(rstd::move(result).unwrap_err()));
        co_return Ok(dbusto::Body::make());
    }
    auto stop() -> void {
        if (rstd::exchange(stopped, true)) return;
        port          = u16();
        host          = None();
        pending_token = None();
        (void)stopped_notice.send_replace(true);
    }
};

struct [[pmacro::attr("dbusto-macros::interface", "io.github.hypengw.Qcm.Daemon")]]
DaemonInterface {
    Rc<DaemonEndpoint> endpoint;

    [[pmacro::helper("dbusto-macros::interface", "method", "GetVersion", "", "s")]]
    auto get_version(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        co_return Ok(dbusto::pack(endpoint->version.clone()));
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "GetPort", "", "q")]]
    auto get_port(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        co_return Ok(dbusto::pack(endpoint->port));
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "OpenUi", "s", "")]]
    auto open_ui(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<String>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        co_return co_await endpoint.get()->open_ui(rstd::move(args->template get<0>()));
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "Next", "", "")]]
    auto next(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return endpoint.get()->action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_NEXT);
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "Prev", "", "")]]
    auto prev(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return endpoint.get()->action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_PREVIOUS);
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "Play", "", "")]]
    auto play(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return endpoint.get()->action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_PLAY);
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "Pause", "", "")]]
    auto pause(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return endpoint.get()->action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_PAUSE);
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "PlayPause", "", "")]]
    auto play_pause(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return endpoint.get()->action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_TOGGLE);
    }
};
} // namespace qcm::backend

export namespace qcm::backend
{
class DaemonBus {
    Rc<DaemonEndpoint>                               endpoint_;
    dbusto::Connection                               connection_;
    bool                                             primary_ {};
    Option<AbortOnDropHandle<dbusto::Result<empty>>> driver_;
    Option<AbortOnDropHandle<empty>>                 incoming_;
    rstd::async::Mutex<empty>                        publishing_ { empty {} };
    rstd::async::Mutex<empty>                        closing_ { empty {} };

    DaemonBus(Rc<DaemonEndpoint> endpoint, dbusto::Connection connection, bool primary,
              AbortOnDropHandle<dbusto::Result<empty>> driver)
        : endpoint_(rstd::move(endpoint)),
          connection_(rstd::move(connection)),
          primary_(primary),
          driver_(Some(rstd::move(driver))) {}
    static auto error(dbusto::Error error) -> String {
        return rstd::format("daemon bus: {}", error);
    }
    static auto drive(Rc<DaemonEndpoint> endpoint, dbusto::Driver driver)
        -> coro<dbusto::Result<empty>> {
        auto result = co_await rstd::move(driver).run();
        endpoint.get()->stop();
        co_return result;
    }
    static auto wait_stopped(rstd::async::watch::Receiver<bool> stopped) -> coro<empty> {
        while (! *stopped.borrow())
            if ((co_await stopped.changed()).is_err()) break;
        co_return empty {};
    }
    static auto dispatch(Rc<DaemonEndpoint> endpoint, dbusto::Connection connection,
                         dbusto::Object object) -> coro<empty> {
        while (! endpoint->stopped) {
            auto call = co_await connection.next();
            if (call.is_err()) break;
            if (call->type == dbusto::MessageType::Signal &&
                call->sender == "org.freedesktop.DBus"_str &&
                call->path == "/org/freedesktop/DBus"_str &&
                call->interface == "org.freedesktop.DBus"_str && call->member == "NameLost"_str) {
                auto name = dbusto::unpack<String>(call->body);
                if (name.is_err() || name->template get<0>() == daemon_bus_name) break;
            }
            if (call->type != dbusto::MessageType::MethodCall) continue;
            if ((co_await object.handle(connection.clone(), rstd::move(*call))).is_err()) break;
        }
        endpoint.get()->stop();
        (void)co_await connection.close();
        co_return empty {};
    }

public:
    // The owner and its host stay on the backend's current-thread runtime.
    static auto claim(String version) -> coro<Result<Rc<DaemonBus>, String>> {
        auto address = rstd::env::var("DBUS_SESSION_BUS_ADDRESS"_str);
        if (address.is_err() || address->is_empty())
            co_return Err(String::make("daemon bus: session bus address is unavailable"_str));
        co_return co_await claim_on_bus(rstd::move(*address), rstd::move(version));
    }
    static auto claim_on_bus(String address, String version)
        -> coro<Result<Rc<DaemonBus>, String>> {
        auto opened     = rstd_co_try(co_await dbusto::connect(address.as_str()), error);
        auto connection = rstd::move(opened.connection);
        auto endpoint   = make_rc<DaemonEndpoint>(rstd::move(version));
        auto driver     = AbortOnDropHandle<dbusto::Result<empty>> { rstd::async::spawn_local(
            drive(endpoint.clone(), rstd::move(opened.driver))) };
        auto named      = co_await connection.claim_name(daemon_bus_name);
        if (named.is_err()) {
            (void)co_await connection.close();
            (void)co_await rstd::move(driver);
            co_return Err(error(rstd::move(named).unwrap_err()));
        }
        auto owner = Rc<DaemonBus>(new DaemonBus(endpoint.clone(),
                                                 rstd::move(connection),
                                                 *named != dbusto::NameClaim::Exists,
                                                 rstd::move(driver)));
        if (owner->primary()) {
            auto object = dbusto::Object::make(daemon_object_path);
            if (object.is_err()) {
                co_await owner.get()->shutdown();
                co_return Err(error(rstd::move(object).unwrap_err()));
            }
            auto added = object->add(DaemonInterface { endpoint.clone() });
            if (added.is_err()) {
                co_await owner.get()->shutdown();
                co_return Err(error(rstd::move(added).unwrap_err()));
            }
            owner.get()->incoming_ = Some(AbortOnDropHandle<empty> { rstd::async::spawn_local(
                dispatch(rstd::move(endpoint), owner->connection_.clone(), rstd::move(*object))) });
        }
        co_return Ok(rstd::move(owner));
    }
    auto primary() const -> bool { return primary_; }
    auto stopped() const -> bool { return endpoint_->stopped; }
    auto notified() const {
        return rstd::async::into_future(wait_stopped(endpoint_->stopped_notice.subscribe()));
    }
    auto forward_open_ui(String token) -> coro<Result<empty, String>> {
        if (endpoint_->stopped) co_return Err(String::make("daemon bus is stopping"_str));
        DaemonInterface::Proxy proxy { connection_.clone(),
                                       String::make(daemon_bus_name),
                                       String::make(daemon_object_path) };
        rstd_co_try(co_await proxy.open_ui(dbusto::pack(rstd::move(token))), error);
        co_return Ok(empty {});
    }
    auto publish(u16 port, Rc<dyn<DaemonHost>> host) -> coro<Result<empty, String>> {
        auto guard = co_await publishing_.lock();
        if (! primary_) co_return Err(String::make("secondary daemon cannot publish"_str));
        if (endpoint_->stopped) co_return Err(String::make("daemon bus is stopping"_str));
        if (port == u16()) co_return Err(String::make("daemon port must be nonzero"_str));
        if (endpoint_->host.is_some())
            co_return Err(String::make("daemon is already published"_str));
        endpoint_.get()->port = port;
        endpoint_.get()->host = Some(host.clone());
        auto pending          = endpoint_.get()->pending_token.take();
        if (pending.is_some()) rstd_co_try(co_await host->open_ui(rstd::move(*pending)));
        if (endpoint_->stopped) co_return Err(String::make("daemon bus is stopping"_str));
        co_return Ok(empty {});
    }
    auto shutdown() -> coro<empty> {
        auto guard = co_await closing_.lock();
        endpoint_.get()->stop();
        (void)co_await connection_.close();
        if (incoming_.is_some()) {
            incoming_->abort();
            (void)co_await rstd::move(*incoming_);
            incoming_ = None();
        }
        if (driver_.is_some()) {
            (void)co_await rstd::move(*driver_);
            driver_ = None();
        }
        co_return empty {};
    }
    ~DaemonBus() { endpoint_.get()->stop(); }
};
} // namespace qcm::backend
