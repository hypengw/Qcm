export module qcm.backend.system.tray:dbusmenu;
export import :model;

using alloc::rc::Rc;
using rstd::async::coro;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::tray
{
struct [[pmacro::attr("dbusto-macros::interface", "com.canonical.dbusmenu")]] MenuInterface {
    Rc<TrayEndpoint> endpoint;

    [[pmacro::helper("dbusto-macros::interface", "property", "Version", "u", "read", "const")]]
    auto version() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::UInt32(u32(3)));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "TextDirection", "s", "read",
                     "const")]]
    auto text_direction() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text("ltr"_Str));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "Status", "s", "read", "const")]]
    auto status() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text("normal"_Str));
    }
    [[pmacro::helper("dbusto-macros::interface", "property", "IconThemePath", "as", "read",
                     "const")]]
    auto icon_theme_path() -> coro<dbusto::Result<dbusto::Value>> {
        Vec<String> paths;
        if (! endpoint->icon_theme_path.is_empty()) paths.push(endpoint->icon_theme_path.clone());
        co_return Ok(dbusto::Codec<Vec<String>>::encode(rstd::move(paths)));
    }

    [[pmacro::helper("dbusto-macros::interface", "method", "GetLayout", "iias", "u(ia{sv}av)")]]
    auto get_layout(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<i32, i32, Vec<String>>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        auto layout = endpoint->model.layout(
            args->template get<0>(), args->template get<1>(), args->template get<2>().as_slice());
        if (layout.is_err()) co_return Err(rstd::move(layout).unwrap_err());
        co_return Ok(dbusto::pack(endpoint->model.revision(), rstd::move(*layout)));
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "GetGroupProperties", "aias",
                     "a(ia{sv})")]]
    auto get_group_properties(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<Vec<i32>, Vec<String>>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        co_return Ok(dbusto::pack(endpoint->model.group_properties(
            args->template get<0>().as_slice(), args->template get<1>().as_slice())));
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "GetProperty", "is", "v")]]
    auto get_property(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<i32, String>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        auto props = endpoint->model.properties(args->template get<0>());
        if (props.is_none()) co_return Err(invalid_item());
        auto value = props->get(args->template get<1>());
        if (value.is_none())
            co_return Err(dbusto::remote_error("org.freedesktop.DBus.Error.InvalidArgs"_str,
                                               "unknown menu property"_str));
        co_return Ok(dbusto::pack((*value)->clone()));
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "Event", "isvu", "")]]
    auto event(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<i32, String, dbusto::Value, u32>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        if (! TrayModel::contains(args->template get<0>())) co_return Err(invalid_item());
        if (args->template get<1>() == "clicked"_str) {
            auto result = co_await endpoint.get()->click(args->template get<0>());
            if (result.is_err()) co_return Err(rstd::move(result).unwrap_err());
        }
        co_return Ok(dbusto::Body::make());
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "EventGroup", "a(isvu)", "ai")]]
    auto event_group(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<Vec<MenuEvent>>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        Vec<i32> errors;
        for (const auto& event : args->template get<0>()) {
            const auto id = event.template get<0>();
            if (! TrayModel::contains(id)) {
                errors.push(i32(id));
                continue;
            }
            if (event.template get<1>() == "clicked"_str) {
                auto result = co_await endpoint.get()->click(id);
                if (result.is_err()) errors.push(i32(id));
            }
        }
        co_return Ok(dbusto::pack(rstd::move(errors)));
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "AboutToShow", "i", "b")]]
    auto about_to_show(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<i32>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        if (! TrayModel::contains(args->template get<0>())) co_return Err(invalid_item());
        co_return Ok(dbusto::pack(false));
    }
    [[pmacro::helper("dbusto-macros::interface", "method", "AboutToShowGroup", "ai", "aiai")]]
    auto about_to_show_group(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<Vec<i32>>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        Vec<i32> errors;
        for (auto id : args->template get<0>())
            if (! TrayModel::contains(id)) errors.push(i32(id));
        co_return Ok(dbusto::pack(Vec<i32> {}, rstd::move(errors)));
    }

    [[pmacro::helper("dbusto-macros::interface", "signal", "ItemsPropertiesUpdated",
                     "a(ia{sv})a(ias)")]]
    static auto items_properties_updated() -> dbusto::SignalSpec;
    [[pmacro::helper("dbusto-macros::interface", "signal", "LayoutUpdated", "ui")]]
    static auto layout_updated() -> dbusto::SignalSpec;
    [[pmacro::helper("dbusto-macros::interface", "signal", "ItemActivationRequested", "iu")]]
    static auto item_activation_requested() -> dbusto::SignalSpec;
};
} // namespace qcm::backend::tray
