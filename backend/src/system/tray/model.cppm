export module qcm.backend.system.tray:model;
import qcm.backend.player.integration;
import proto.p_message;
import rstd;
import dbusto;

using alloc::rc::Rc;
using rstd::async::coro;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::tray
{
inline constexpr auto item_path       = "/StatusNotifierItem"_str;
inline constexpr auto menu_path       = "/MenuBar"_str;
inline constexpr auto item_interface  = "org.kde.StatusNotifierItem"_str;
inline constexpr auto menu_interface  = "com.canonical.dbusmenu"_str;
inline constexpr auto watcher_service = "org.kde.StatusNotifierWatcher"_str;
inline constexpr auto watcher_path    = "/StatusNotifierWatcher"_str;

enum class MenuId : int
{
    Root              = 0,
    OpenUi            = 1,
    Next              = 2,
    Previous          = 3,
    OpenSeparator     = 4,
    PlayPause         = 5,
    PlaybackSeparator = 6,
    QuitSeparator     = 9,
    Quit              = 10,
    Shuffle           = 11,
    Repeat            = 20,
    RepeatOff         = 21,
    RepeatOne         = 22,
    RepeatAll         = 23
};

inline constexpr array<MenuId, 10> root_items { MenuId::OpenUi,        MenuId::OpenSeparator,
                                                MenuId::Previous,      MenuId::PlayPause,
                                                MenuId::Next,          MenuId::PlaybackSeparator,
                                                MenuId::Shuffle,       MenuId::Repeat,
                                                MenuId::QuitSeparator, MenuId::Quit };
inline constexpr array<MenuId, 3>  repeat_items { MenuId::RepeatOff,
                                                  MenuId::RepeatOne,
                                                  MenuId::RepeatAll };
inline constexpr array<MenuId, 14> all_items { MenuId::Root,
                                               MenuId::OpenUi,
                                               MenuId::Next,
                                               MenuId::Previous,
                                               MenuId::OpenSeparator,
                                               MenuId::PlayPause,
                                               MenuId::PlaybackSeparator,
                                               MenuId::QuitSeparator,
                                               MenuId::Quit,
                                               MenuId::Shuffle,
                                               MenuId::Repeat,
                                               MenuId::RepeatOff,
                                               MenuId::RepeatOne,
                                               MenuId::RepeatAll };
using Layout            = rstd::tuple<i32, dbusto::PropertyMap, Vec<dbusto::Value>>;
using ItemProperties    = rstd::tuple<i32, dbusto::PropertyMap>;
using RemovedProperties = rstd::tuple<i32, Vec<String>>;
using MenuEvent         = rstd::tuple<i32, String, dbusto::Value, u32>;
using Pixmap            = rstd::tuple<i32, i32, rstd::bytes::Bytes>;
using ToolTip           = rstd::tuple<String, Vec<Pixmap>, String, String>;

inline auto number(MenuId id) -> i32 { return i32(static_cast<int>(id)); }
inline auto invalid_item() -> dbusto::Error {
    return dbusto::remote_error("org.freedesktop.DBus.Error.InvalidArgs"_str,
                                "unknown menu item"_str);
}
inline auto failed(String reason) -> dbusto::Error {
    return dbusto::Error::Remote("org.freedesktop.DBus.Error.Failed"_Str, rstd::move(reason));
}

struct MenuState {
    bool connected {}, playing {}, playable {}, next {}, previous {}, shuffle {};
    i32  loop {};
    auto operator==(const MenuState&) const -> bool = default;
};

class TrayModel {
    MenuState state_;
    bool      track_ {}, queued_ {}, received_state_ {}, received_queue_ {};
    u64       state_revision_ {}, epoch_ {}, queue_revision_ {};
    u32       revision_ { 1 };

    auto changed(MenuState before) -> bool {
        state_.playable = state_.connected && (track_ || queued_);
        if (before == state_) return false;
        revision_ = revision_.wrapping_add(u32(1));
        if (revision_ == u32()) revision_ = u32(1);
        return true;
    }

public:
    auto state() const -> const MenuState& { return state_; }
    auto revision() const -> u32 { return revision_; }
    auto update_state(const p_qcm::p_msg::M_PlayerStateMsg& value) -> bool {
        if (received_state_ &&
            (value.f_revision < state_revision_ || value.f_source_epoch < epoch_))
            return false;
        const auto before = state_;
        received_state_   = true;
        state_revision_   = value.f_revision;
        epoch_            = value.f_source_epoch;
        state_.connected  = value.f_connected;
        track_            = ! value.f_source.is_empty() || value.f_song_id.is_some();
        state_.playing    = value.f_playback_state == i32() && value.f_connected && track_;
        return changed(before);
    }
    auto update_queue(const p_qcm::p_msg::M_PlayerQueueMsg& value) -> bool {
        if (received_queue_ && value.f_revision < queue_revision_) return false;
        const auto before = state_;
        received_queue_   = true;
        queue_revision_   = value.f_revision;
        state_.shuffle    = value.f_shuffle;
        state_.loop       = value.f_loop_mode;
        state_.next       = value.f_can_next;
        state_.previous   = value.f_can_previous;
        queued_           = ! value.f_song_ids.is_empty();
        return changed(before);
    }
    static auto contains(i32 id) -> bool {
        for (auto item : all_items)
            if (number(item) == id) return true;
        return false;
    }
    auto enabled(i32 id) const -> bool {
        switch (static_cast<MenuId>(id.to_primitive())) {
        case MenuId::Previous: return state_.connected && state_.previous;
        case MenuId::Next: return state_.connected && state_.next;
        case MenuId::PlayPause: return state_.playable;
        default: return contains(id);
        }
    }
    auto properties(i32 id, slice<String> names = {}) const -> Option<dbusto::PropertyMap> {
        if (! contains(id)) return None();
        auto       props = dbusto::PropertyMap::make();
        const auto item  = static_cast<MenuId>(id.to_primitive());
        ref<str>   label = ""_str;
        switch (item) {
        case MenuId::Root: break;
        case MenuId::OpenUi: label = "Open UI"_str; break;
        case MenuId::Next: label = "Next"_str; break;
        case MenuId::Previous: label = "Previous"_str; break;
        case MenuId::PlayPause: label = state_.playing ? "Pause"_str : "Play"_str; break;
        case MenuId::Quit: label = "Quit"_str; break;
        case MenuId::Shuffle: label = "Shuffle"_str; break;
        case MenuId::Repeat: label = "Repeat"_str; break;
        case MenuId::RepeatOff: label = "Off"_str; break;
        case MenuId::RepeatOne: label = "One"_str; break;
        case MenuId::RepeatAll: label = "All"_str; break;
        case MenuId::OpenSeparator:
        case MenuId::PlaybackSeparator:
        case MenuId::QuitSeparator:
            props.insert("type"_Str, dbusto::Value::Text("separator"_Str));
            break;
        }
        if (! label.is_empty()) props.insert("label"_Str, dbusto::Value::Text(String::make(label)));
        if (item == MenuId::Root || item == MenuId::Repeat)
            props.insert("children-display"_Str, dbusto::Value::Text("submenu"_Str));
        props.insert("enabled"_Str, dbusto::Value::Boolean(enabled(id)));
        props.insert("visible"_Str, dbusto::Value::Boolean(true));
        if (item == MenuId::Shuffle) {
            props.insert("toggle-type"_Str, dbusto::Value::Text("checkmark"_Str));
            props.insert("toggle-state"_Str, dbusto::Value::Int32(i32(state_.shuffle ? 1 : 0)));
        }
        if (item == MenuId::RepeatOff || item == MenuId::RepeatOne || item == MenuId::RepeatAll) {
            props.insert("toggle-type"_Str, dbusto::Value::Text("radio"_Str));
            props.insert(
                "toggle-state"_Str,
                dbusto::Value::Int32(i32(state_.loop == id - number(MenuId::RepeatOff) ? 1 : 0)));
        }
        if (names.is_empty()) return Some(rstd::move(props));
        auto filtered = dbusto::PropertyMap::make();
        for (const auto& name : names) {
            auto value = props.get(name);
            if (value.is_some()) filtered.insert(name.clone(), (*value)->clone());
        }
        return Some(rstd::move(filtered));
    }
    auto layout(i32 id, i32 depth, slice<String> names = {}) const -> dbusto::Result<Layout> {
        if (depth < i32(-1)) return Err(dbusto::type_error());
        auto props = properties(id, names);
        if (props.is_none()) return Err(invalid_item());
        Vec<dbusto::Value> children;
        if (depth != i32()) {
            const auto next_depth = depth < i32() ? depth : depth - i32(1);
            const auto append     = [&](auto items) {
                for (auto item : items)
                    children.push(dbusto::Codec<Layout>::encode(
                        layout(number(item), next_depth, names).unwrap()));
            };
            if (id == number(MenuId::Root)) append(root_items.as_slice());
            if (id == number(MenuId::Repeat)) append(repeat_items.as_slice());
        }
        return Ok(Layout { id, rstd::move(*props), rstd::move(children) });
    }
    auto group_properties(slice<i32> ids, slice<String> names) const -> Vec<ItemProperties> {
        Vec<ItemProperties> result;
        const auto          append = [&](i32 id) {
            auto props = properties(id, names);
            if (props.is_some()) result.push(ItemProperties { id, rstd::move(*props) });
        };
        if (ids.is_empty())
            for (auto id : all_items) append(number(id));
        else
            for (auto id : ids) append(id);
        return result;
    }
};

struct TrayEndpoint {
    Rc<dyn<PlayerHost>> host;
    TrayModel           model;
    String              icon_theme_path;
    Option<String>      activation_token;
    bool                stopped {}, quit_requested {};

    auto open_ui() -> coro<dbusto::Result<empty>> {
        if (stopped) co_return Err(failed("tray is stopping"_Str));
        auto token  = rstd::move(activation_token.take()).unwrap_or(String {});
        auto result = co_await host->raise(rstd::move(token));
        if (result.is_err()) co_return Err(failed(rstd::move(result).unwrap_err()));
        co_return Ok(empty {});
    }
    auto action(p_qcm::p_msg::E_PlayerAction action) -> coro<dbusto::Result<empty>> {
        if (stopped) co_return Err(failed("tray is stopping"_Str));
        p_qcm::p_msg::M_PlayerControlReq request;
        request.f_action = i32(static_cast<int>(action));
        auto result      = co_await host->control(rstd::move(request));
        if (result.is_err()) co_return Err(failed(rstd::move(result).unwrap_err()));
        co_return Ok(empty {});
    }
    auto click(i32 id) -> coro<dbusto::Result<empty>> {
        if (stopped) co_return Err(failed("tray is stopping"_Str));
        if (! TrayModel::contains(id)) co_return Err(invalid_item());
        if (! model.enabled(id)) co_return Ok(empty {});
        p_qcm::p_msg::M_PlayerQueueReq request;
        switch (static_cast<MenuId>(id.to_primitive())) {
        case MenuId::OpenUi: co_return co_await open_ui();
        case MenuId::Previous:
            co_return co_await action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_PREVIOUS);
        case MenuId::Next:
            co_return co_await action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_NEXT);
        case MenuId::PlayPause:
            co_return co_await action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_TOGGLE);
        case MenuId::Quit: quit_requested = true; co_return Ok(empty {});
        case MenuId::Shuffle:
            request.f_action = i32(
                static_cast<int>(p_qcm::p_msg::E_PlayerQueueAction::v_PLAYER_QUEUE_ACTION_SHUFFLE));
            request.f_shuffle = Some(! model.state().shuffle);
            break;
        case MenuId::RepeatOff:
        case MenuId::RepeatOne:
        case MenuId::RepeatAll:
            request.f_action = i32(
                static_cast<int>(p_qcm::p_msg::E_PlayerQueueAction::v_PLAYER_QUEUE_ACTION_LOOP));
            request.f_loop_mode = id - number(MenuId::RepeatOff);
            break;
        default: co_return Ok(empty {});
        }
        auto result = co_await host->queue(rstd::move(request));
        if (result.is_err()) co_return Err(failed(rstd::move(result).unwrap_err()));
        co_return Ok(empty {});
    }
    auto after_reply() -> coro<PlayerIntegrationResult> {
        if (! rstd::exchange(quit_requested, false) || stopped) co_return Ok(empty {});
        co_return co_await host->quit();
    }
};
} // namespace qcm::backend::tray
