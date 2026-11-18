module;
#include <rstd/macro.hpp>
export module qcm.backend.system.dbus;
import proto.p_message;
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
    static auto failed(String reason) -> dbusto::Error;
    static auto stopping() -> dbusto::Error;
    auto        open_ui(String token) -> coro<dbusto::Result<dbusto::Body>>;
    auto        action(p_qcm::p_msg::E_PlayerAction action) -> coro<dbusto::Result<dbusto::Body>>;
    auto        stop() -> void;
};

struct [[pmacro::attr("dbusto-macro::interface", "io.github.hypengw.Qcm.Daemon")]] DaemonInterface {
    Rc<DaemonEndpoint> endpoint;

    [[pmacro::helper("dbusto-macro::interface", "method", "GetVersion", "", "s")]]
    auto get_version(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        co_return Ok(dbusto::pack(endpoint->version.clone()));
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "GetPort", "", "q")]]
    auto get_port(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        co_return Ok(dbusto::pack(endpoint->port));
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "OpenUi", "s", "")]]
    auto open_ui(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<String>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        co_return co_await endpoint.get()->open_ui(rstd::move(args->template get<0>()));
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "Next", "", "")]]
    auto next(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return endpoint.get()->action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_NEXT);
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "Prev", "", "")]]
    auto prev(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return endpoint.get()->action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_PREVIOUS);
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "Play", "", "")]]
    auto play(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return endpoint.get()->action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_PLAY);
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "Pause", "", "")]]
    auto pause(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return endpoint.get()->action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_PAUSE);
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "PlayPause", "", "")]]
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
              AbortOnDropHandle<dbusto::Result<empty>> driver);
    static auto error(dbusto::Error error) -> String;
    static auto drive(Rc<DaemonEndpoint> endpoint, dbusto::Driver driver)
        -> coro<dbusto::Result<empty>>;
    static auto wait_stopped(rstd::async::watch::Receiver<bool> stopped) -> coro<empty>;
    static auto dispatch(Rc<DaemonEndpoint> endpoint, dbusto::Connection connection,
                         dbusto::Object object) -> coro<empty>;

public:
    // The owner and its host stay on the backend's current-thread runtime.
    static auto claim(String version) -> coro<Result<Rc<DaemonBus>, String>>;
    static auto claim_on_bus(String address, String version) -> coro<Result<Rc<DaemonBus>, String>>;
    auto        primary() const -> bool;
    auto        stopped() const -> bool;
    auto        notified() const {
        return rstd::async::into_future(wait_stopped(endpoint_->stopped_notice.subscribe()));
    }
    auto forward_open_ui(String token) -> coro<Result<empty, String>>;
    auto publish(u16 port, Rc<dyn<DaemonHost>> host) -> coro<Result<empty, String>>;
    auto shutdown() -> coro<empty>;
    ~DaemonBus();
};
} // namespace qcm::backend
