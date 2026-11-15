export module qcm.backend.system.tray:sni;
export import :model;

using alloc::rc::Rc;
using rstd::async::coro;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::tray
{
inline auto completed(coro<dbusto::Result<empty>> operation) -> coro<dbusto::Result<dbusto::Body>> {
    auto result = co_await rstd::move(operation);
    if (result.is_err()) co_return Err(rstd::move(result).unwrap_err());
    co_return Ok(dbusto::Body::make());
}

struct [[pmacro::attr("dbusto-macros::interface", "org.kde.StatusNotifierItem")]] ItemInterface {
    Rc<TrayEndpoint> endpoint;

    [[pmacro::helper("dbusto-macros::interface", "property", "Category", "s", "read", "const")]]
    auto category() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text("ApplicationStatus"_Str));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "Id", "s", "read", "const")]]
    auto id() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text("Qcm"_Str));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "Title", "s", "read", "false")]]
    auto title() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text("Qcm"_Str));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "Status", "s", "read", "false")]]
    auto status() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text("Active"_Str));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "WindowId", "u", "read", "const")]]
    auto window_id() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::UInt32(u32()));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "IconName", "s", "read", "false")]]
    auto icon_name() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text("io.github.hypengw.Qcm"_Str));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "IconThemePath", "s", "read",
                     "const")]]
    auto icon_theme_path() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text(endpoint->icon_theme_path.clone()));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "IconPixmap", "a(iiay)", "read",
                     "false")]]
    auto icon_pixmap() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Codec<Vec<Pixmap>>::encode({}));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "OverlayIconName", "s", "read",
                     "false")]]
    auto overlay_icon_name() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text(String {}));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "OverlayIconPixmap", "a(iiay)", "read",
                     "false")]]
    auto overlay_icon_pixmap() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Codec<Vec<Pixmap>>::encode({}));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "AttentionIconName", "s", "read",
                     "false")]]
    auto attention_icon_name() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text(String {}));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "AttentionIconPixmap", "a(iiay)",
                     "read", "false")]]
    auto attention_icon_pixmap() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Codec<Vec<Pixmap>>::encode({}));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "AttentionMovieName", "s", "read",
                     "false")]]
    auto attention_movie_name() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text(String {}));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "ToolTip", "(sa(iiay)ss)", "read",
                     "false")]]
    auto tool_tip() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Codec<ToolTip>::encode(
            ToolTip { String {}, Vec<Pixmap> {}, "Qcm"_Str, "Music player"_Str }));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "ItemIsMenu", "b", "read", "const")]]
    auto item_is_menu() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(true));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "Menu", "o", "read", "const")]]
    auto menu() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Path(String::make(menu_path)));
    }

    [[pmacro::helper("dbusto-macros::interface", "method", "Activate", "ii", "")]]
    auto activate(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return completed(endpoint.get()->open_ui());
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "SecondaryActivate", "ii", "")]]
    auto secondary_activate(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return completed(endpoint.get()->click(number(MenuId::Next)));
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "ContextMenu", "ii", "")]]
    auto context_menu(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        co_return Ok(dbusto::Body::make());
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "Scroll", "is", "")]]
    auto scroll(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<i32, String>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        co_return co_await completed(endpoint.get()->click(
            number(args->template get<0>() >= i32() ? MenuId::Next : MenuId::Previous)));
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "ProvideXdgActivationToken", "s", "")]]
    auto provide_token(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<String>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        if (endpoint->stopped) co_return Err(failed("tray is stopping"_Str));
        endpoint.get()->activation_token = Some(rstd::move(args->template get<0>()));
        co_return Ok(dbusto::Body::make());
    }

    [[pmacro::helper("dbusto-macros::interface", "signal", "NewTitle", "")]]
    static auto new_title() -> dbusto::SignalSpec;
    [[pmacro::helper("dbusto-macros::interface", "signal", "NewIcon", "")]]
    static auto new_icon() -> dbusto::SignalSpec;
    [[pmacro::helper("dbusto-macros::interface", "signal", "NewAttentionIcon", "")]]
    static auto new_attention_icon() -> dbusto::SignalSpec;
    [[pmacro::helper("dbusto-macros::interface", "signal", "NewOverlayIcon", "")]]
    static auto new_overlay_icon() -> dbusto::SignalSpec;
    [[pmacro::helper("dbusto-macros::interface", "signal", "NewMenu", "")]]
    static auto new_menu() -> dbusto::SignalSpec;
    [[pmacro::helper("dbusto-macros::interface", "signal", "NewToolTip", "")]]
    static auto new_tool_tip() -> dbusto::SignalSpec;
    [[pmacro::helper("dbusto-macros::interface", "signal", "NewStatus", "s")]]
    static auto new_status() -> dbusto::SignalSpec;
};
} // namespace qcm::backend::tray
