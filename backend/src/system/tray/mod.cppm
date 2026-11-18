module;
#include <rstd/macro.hpp>
export module qcm.backend.system.tray;
import :model;
import qcm.backend.player.integration;
import dbusto;
import rstd;

using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using rstd::path::Path;
using rstd::path::PathBuf;
using rstd::time::Duration;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
auto tray_icon_theme_path(ref<Path> executable) -> String;

class DesktopTray {
    Rc<tray::TrayEndpoint>                           endpoint_;
    dbusto::Connection                               connection_;
    dbusto::Object                                   item_, menu_;
    rstd::async::Notify                              registration_;
    Option<AbortOnDropHandle<dbusto::Result<empty>>> driver_;
    Option<AbortOnDropHandle<empty>>                 incoming_, watcher_;
    rstd::async::Mutex<empty>                        updates_ { empty {} }, closing_ { empty {} };

    DesktopTray(Rc<tray::TrayEndpoint> endpoint, dbusto::Connection connection, dbusto::Object item,
                dbusto::Object menu, rstd::async::Notify registration,
                AbortOnDropHandle<dbusto::Result<empty>> driver);
    static auto error(dbusto::Error error) -> String;
    static auto register_watcher(Rc<tray::TrayEndpoint> endpoint, dbusto::Connection connection,
                                 rstd::async::Notify requested) -> coro<empty>;
    static auto dispatch(Rc<tray::TrayEndpoint> endpoint, dbusto::Connection connection,
                         dbusto::Object item, dbusto::Object menu, rstd::async::Notify requested)
        -> coro<empty>;
    auto publish() -> coro<PlayerIntegrationResult>;

public:
    static auto start(Rc<dyn<PlayerHost>> host, String icon_theme_path = {})
        -> coro<Result<Rc<DesktopTray>, String>>;
    static auto start_on_bus(String address, Rc<dyn<PlayerHost>> host, String icon_theme_path = {})
        -> coro<Result<Rc<DesktopTray>, String>>;
    auto unique_name() const -> String;
    auto update_state(PlayerStateSnapshot state) -> coro<PlayerIntegrationResult>;
    auto update_queue(PlayerQueueSnapshot queue) -> coro<PlayerIntegrationResult>;
    auto shutdown() -> coro<empty>;
    ~DesktopTray();
};
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<qcm::backend::PlayerIntegration, Rc<qcm::backend::DesktopTray>>
    : ImplBase<Rc<qcm::backend::DesktopTray>> {
    auto update_state(qcm::backend::PlayerStateSnapshot state)
        -> coro<qcm::backend::PlayerIntegrationResult> {
        return this->self().get()->update_state(rstd::move(state));
    }
    auto update_queue(qcm::backend::PlayerQueueSnapshot queue)
        -> coro<qcm::backend::PlayerIntegrationResult> {
        return this->self().get()->update_queue(rstd::move(queue));
    }
    auto shutdown() -> coro<empty> { return this->self().get()->shutdown(); }
};
} // namespace rstd
