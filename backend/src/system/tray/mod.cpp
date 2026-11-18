module;
#include <rstd/macro.hpp>
module qcm.backend.system.tray;
import rstd;
import dbusto;
import proto.p_message;
import qcm.backend.player.integration;
import :model;
import :sni;
import :dbusmenu;
import rstd.log;

using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using rstd::path::Path;
using rstd::path::PathBuf;
using rstd::time::Duration;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
auto tray_icon_theme_path(ref<Path> executable) -> String {
    auto bin = executable.parent();
    if (bin.is_none()) return {};
    auto prefix = bin->parent();
    if (prefix.is_none()) return {};
    auto icons    = PathBuf::from(*prefix).join("share/icons"_str);
    auto resolved = rstd::fs::canonicalize(icons.as_path());
    if (resolved.is_err()) return {};
    auto text = resolved->as_path().to_str();
    return text.is_some() ? String::make(*text) : String {};
}

DesktopTray::DesktopTray(Rc<tray::TrayEndpoint> endpoint, dbusto::Connection connection,
                         dbusto::Object item, dbusto::Object menu, rstd::async::Notify registration,
                         AbortOnDropHandle<dbusto::Result<empty>> driver)
    : endpoint_(rstd::move(endpoint)),
      connection_(rstd::move(connection)),
      item_(rstd::move(item)),
      menu_(rstd::move(menu)),
      registration_(rstd::move(registration)),
      driver_(Some(rstd::move(driver))) {}

auto DesktopTray::error(dbusto::Error error) -> String { return rstd::format("tray: {}", error); }

auto DesktopTray::register_watcher(Rc<tray::TrayEndpoint> endpoint, dbusto::Connection connection,
                                   rstd::async::Notify requested) -> coro<empty> {
    for (;;) {
        co_await requested.notified();
        if (endpoint->stopped) co_return empty {};
        auto result = co_await connection.call(
            dbusto::method_call(tray::watcher_service,
                                tray::watcher_path,
                                tray::watcher_service,
                                "RegisterStatusNotifierItem"_str,
                                dbusto::pack(String::make(tray::item_path))));
        if (result.is_err())
            rstd::log::info("tray watcher registration deferred: {}", result.unwrap_err());
        else
            rstd::log::info("tray registered {}", tray::item_path);
    }
}

auto DesktopTray::dispatch(Rc<tray::TrayEndpoint> endpoint, dbusto::Connection connection,
                           dbusto::Object item, dbusto::Object menu, rstd::async::Notify requested)
    -> coro<empty> {
    while (! endpoint->stopped) {
        auto call = co_await connection.next();
        if (call.is_err()) break;
        if (call->type == dbusto::MessageType::Signal &&
            call->sender == "org.freedesktop.DBus"_str &&
            call->path == "/org/freedesktop/DBus"_str &&
            call->interface == "org.freedesktop.DBus"_str &&
            call->member == "NameOwnerChanged"_str) {
            auto args = dbusto::unpack<String, String, String>(call->body);
            if (args.is_ok() && args->template get<0>() == tray::watcher_service &&
                ! args->template get<2>().is_empty())
                (void)requested.notifier().notify();
            continue;
        }
        if (call->type != dbusto::MessageType::MethodCall) continue;
        auto& object  = call->path == tray::menu_path ? menu : item;
        auto  handled = co_await object.handle(connection.clone(), rstd::move(*call));
        if (handled.is_err()) break;
        auto quit = co_await endpoint.get()->after_reply();
        if (quit.is_err()) rstd::log::warn("tray quit failed: {}", quit.unwrap_err());
    }
    endpoint.get()->stopped = true;
    (void)requested.notifier().notify();
    (void)co_await connection.close();
    co_return empty {};
}

auto DesktopTray::publish() -> coro<PlayerIntegrationResult> {
    auto properties = endpoint_->model.group_properties({}, {});
    auto updated    = rstd_co_try(
        dbusto::signal(tray::menu_path,
                       tray::menu_interface,
                       tray::MenuInterface::items_properties_updated(),
                       dbusto::pack(rstd::move(properties), Vec<tray::RemovedProperties> {})),
        error);
    rstd_co_try(co_await connection_.send(rstd::move(updated)), error);
    auto layout = rstd_co_try(
        dbusto::signal(tray::menu_path,
                       tray::menu_interface,
                       tray::MenuInterface::layout_updated(),
                       dbusto::pack(endpoint_->model.revision(), tray::number(tray::MenuId::Root))),
        error);
    rstd_co_try(co_await connection_.send(rstd::move(layout)), error);
    co_return Ok(empty {});
}

auto DesktopTray::start(Rc<dyn<PlayerHost>> host, String icon_theme_path)
    -> coro<Result<Rc<DesktopTray>, String>> {
    auto address = rstd::env::var("DBUS_SESSION_BUS_ADDRESS"_str);
    if (address.is_err() || address->is_empty())
        co_return Err("tray: session bus is unavailable"_Str);
    co_return co_await start_on_bus(
        rstd::move(*address), rstd::move(host), rstd::move(icon_theme_path));
}

auto DesktopTray::start_on_bus(String address, Rc<dyn<PlayerHost>> host, String icon_theme_path)
    -> coro<Result<Rc<DesktopTray>, String>> {
    auto endpoint = make_rc<tray::TrayEndpoint>(
        tray::TrayEndpoint { rstd::move(host), {}, rstd::move(icon_theme_path), {}, false, false });
    auto item = rstd_co_try(dbusto::Object::make(tray::item_path), error);
    auto menu = rstd_co_try(dbusto::Object::make(tray::menu_path), error);
    rstd_co_try(item.add(tray::ItemInterface { endpoint.clone() }), error);
    rstd_co_try(menu.add(tray::MenuInterface { endpoint.clone() }), error);
    auto registration = rstd_co_try(rstd::async::Notify::make(), [](auto error) {
        return rstd::format("tray notification: {}", error);
    });
    dbusto::ConnectionOptions options;
    options.call_timeout = Duration::from_secs(u64(5));
    auto opened          = rstd_co_try(co_await dbusto::connect(address.as_str(), options), error);
    auto connection      = rstd::move(opened.connection);
    auto driver          = AbortOnDropHandle<dbusto::Result<empty>> { rstd::async::spawn_local(
        rstd::move(opened.driver).run()) };
    auto watched         = co_await connection.add_match(
        "type='signal',sender='org.freedesktop.DBus',path='/org/freedesktop/DBus',"
        "interface='org.freedesktop.DBus',member='NameOwnerChanged',arg0='org.kde.StatusNotifierWatcher'"_str);
    if (watched.is_err()) {
        (void)co_await connection.close();
        (void)co_await rstd::move(driver);
        co_return Err(error(rstd::move(watched).unwrap_err()));
    }
    auto owner             = Rc<DesktopTray>(new DesktopTray(endpoint.clone(),
                                                             rstd::move(connection),
                                                             rstd::move(item),
                                                             rstd::move(menu),
                                                             registration.clone(),
                                                             rstd::move(driver)));
    owner.get()->incoming_ = Some(
        AbortOnDropHandle<empty> { rstd::async::spawn_local(dispatch(endpoint.clone(),
                                                                     owner->connection_.clone(),
                                                                     owner->item_.clone(),
                                                                     owner->menu_.clone(),
                                                                     registration.clone())) });
    owner.get()->watcher_ = Some(AbortOnDropHandle<empty> { rstd::async::spawn_local(
        register_watcher(endpoint.clone(), owner->connection_.clone(), registration.clone())) });
    (void)registration.notifier().notify();
    co_return Ok(rstd::move(owner));
}

auto DesktopTray::unique_name() const -> String { return connection_.unique_name(); }

auto DesktopTray::update_state(PlayerStateSnapshot state) -> coro<PlayerIntegrationResult> {
    auto guard = co_await updates_.lock();
    if (endpoint_->stopped) co_return Err("tray is stopping"_Str);
    if (! endpoint_.get()->model.update_state(*state)) co_return Ok(empty {});
    co_return co_await publish();
}

auto DesktopTray::update_queue(PlayerQueueSnapshot queue) -> coro<PlayerIntegrationResult> {
    auto guard = co_await updates_.lock();
    if (endpoint_->stopped) co_return Err("tray is stopping"_Str);
    if (! endpoint_.get()->model.update_queue(*queue)) co_return Ok(empty {});
    co_return co_await publish();
}

auto DesktopTray::shutdown() -> coro<empty> {
    auto guard                        = co_await closing_.lock();
    endpoint_.get()->stopped          = true;
    endpoint_.get()->activation_token = None();
    (void)registration_.notifier().notify();
    (void)co_await connection_.close();
    if (incoming_.is_some()) {
        incoming_->abort();
        (void)co_await rstd::move(*incoming_);
        incoming_ = None();
    }
    if (watcher_.is_some()) {
        watcher_->abort();
        (void)co_await rstd::move(*watcher_);
        watcher_ = None();
    }
    if (driver_.is_some()) {
        (void)co_await rstd::move(*driver_);
        driver_ = None();
    }
    co_return empty {};
}

DesktopTray::~DesktopTray() { endpoint_.get()->stopped = true; }
} // namespace qcm::backend
