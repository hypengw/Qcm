module;
#include <rstd/macro.hpp>
export module qcm.backend.system.mpris;
import proto.p_message;
import qcm.backend.player.integration;
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
using MprisResult                            = PlayerIntegrationResult;
inline constexpr auto mpris_bus_name         = "org.mpris.MediaPlayer2.Qcm"_str;
inline constexpr auto mpris_object_path      = "/org/mpris/MediaPlayer2"_str;
inline constexpr auto mpris_root_interface   = "org.mpris.MediaPlayer2"_str;
inline constexpr auto mpris_player_interface = "org.mpris.MediaPlayer2.Player"_str;
inline constexpr auto mpris_no_track         = "/org/mpris/MediaPlayer2/TrackList/NoTrack"_str;

struct MprisChanges {
    dbusto::PropertyMap properties = dbusto::PropertyMap::make();
    Option<i64>         seeked;
};

class MprisModel {
    PlayerStateSnapshot state_ = PlayerStateSnapshot::make();
    u64                 queue_revision_ {};
    i32                 loop_mode_ {};
    bool                shuffle_ {};
    bool                can_next_ {};
    bool                can_previous_ {};
    bool                queued_tracks_ {};
    bool                received_state_ {};
    bool                received_queue_ {};

    static auto microseconds(i64 milliseconds) -> i64;
    static auto metadata_changed(const p_qcm::p_msg::M_PlayerStateMsg& previous,
                                 const p_qcm::p_msg::M_PlayerStateMsg& next) -> bool;

public:
    auto has_track() const -> bool;
    auto connected() const -> bool;
    auto source_epoch() const -> u64;
    auto can_play() const -> bool;
    auto can_pause() const -> bool;
    auto can_seek() const -> bool;
    auto can_next() const -> bool;
    auto can_previous() const -> bool;
    auto shuffle() const -> bool;
    auto volume() const -> double;
    auto duration() const -> i64;
    auto position() const -> i64;
    auto track_id() const -> String;
    auto playback_status() const -> ref<str>;
    auto loop_status() const -> ref<str>;
    auto metadata() const -> dbusto::PropertyMap;
    auto update_state(PlayerStateSnapshot snapshot) -> MprisChanges;
    auto update_state(p_qcm::p_msg::M_PlayerStateMsg state) -> MprisChanges;
    auto update_queue(const p_qcm::p_msg::M_PlayerQueueMsg& queue) -> MprisChanges;
    auto seek(i64 offset) const -> Option<p_qcm::p_msg::M_PlayerControlReq>;
    auto set_position(ref<str> track, i64 position) const
        -> Option<p_qcm::p_msg::M_PlayerControlReq>;
};

struct MprisEndpoint {
    Rc<dyn<PlayerHost>> host;
    MprisModel          model;
    bool                stopped {};
    bool                quit_requested {};

    explicit MprisEndpoint(Rc<dyn<PlayerHost>> host): host(rstd::move(host)) {}
    static auto failed(String reason) -> dbusto::Error;
    auto control(p_qcm::p_msg::M_PlayerControlReq command) -> coro<dbusto::Result<dbusto::Body>>;
    auto action(p_qcm::p_msg::E_PlayerAction action) -> coro<dbusto::Result<dbusto::Body>>;
    auto queue(p_qcm::p_msg::M_PlayerQueueReq command) -> coro<dbusto::Result<empty>>;
    auto raise() -> coro<dbusto::Result<dbusto::Body>>;
    auto after_reply() -> coro<MprisResult>;
    static auto completed(coro<dbusto::Result<dbusto::Body>> command)
        -> coro<dbusto::Result<empty>>;
};

struct [[pmacro::attr("dbusto-macro::interface", "org.mpris.MediaPlayer2")]] MprisRootInterface {
    Rc<MprisEndpoint> endpoint;

    [[pmacro::helper("dbusto-macro::interface", "method", "Raise", "", "")]]
    auto raise(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return endpoint.get()->raise();
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "Quit", "", "")]]
    auto quit(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        if (endpoint->stopped)
            co_return Err(MprisEndpoint::failed(String::make("MPRIS is stopping"_str)));
        endpoint.get()->quit_requested = true;
        co_return Ok(dbusto::Body::make());
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "CanQuit", "b", "read", "const")]]
    auto can_quit() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(true));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "CanRaise", "b", "read", "const")]]
    auto can_raise() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(true));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "HasTrackList", "b", "read", "const")]]
    auto has_track_list() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(false));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "Fullscreen", "b", "readwrite", "false",
                     "set_fullscreen")]]
    auto fullscreen() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(false));
    }
    auto set_fullscreen(const dbusto::Value&) -> coro<dbusto::Result<empty>> {
        co_return Err(dbusto::remote_error("org.freedesktop.DBus.Error.NotSupported"_str,
                                           "fullscreen is not supported"_str));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "CanSetFullscreen", "b", "read",
                     "const")]]
    auto can_set_fullscreen() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(false));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "Identity", "s", "read", "const")]]
    auto identity() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text(String::make("Qcm"_str)));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "DesktopEntry", "s", "read", "const")]]
    auto desktop_entry() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text(String::make("io.github.hypengw.Qcm"_str)));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "SupportedUriSchemes", "as", "read",
                     "const")]]
    auto supported_uri_schemes() -> coro<dbusto::Result<dbusto::Value>> {
        auto                     values = Vec<String>::make();
        const array<ref<str>, 3> schemes { "file"_str, "http"_str, "https"_str };
        for (auto value : schemes) values.push(String::make(value));
        co_return Ok(dbusto::Codec<Vec<String>>::encode(rstd::move(values)));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "SupportedMimeTypes", "as", "read",
                     "const")]]
    auto supported_mime_types() -> coro<dbusto::Result<dbusto::Value>> {
        auto                     values = Vec<String>::make();
        const array<ref<str>, 8> types { "audio/mpeg"_str,      "audio/flac"_str, "audio/ogg"_str,
                                         "application/ogg"_str, "audio/wav"_str,  "audio/mp4"_str,
                                         "audio/aac"_str,       "audio/opus"_str };
        for (auto value : types) values.push(String::make(value));
        co_return Ok(dbusto::Codec<Vec<String>>::encode(rstd::move(values)));
    }
};

struct [[pmacro::attr("dbusto-macro::interface", "org.mpris.MediaPlayer2.Player")]]
MprisPlayerInterface {
    Rc<MprisEndpoint> endpoint;

    [[pmacro::helper("dbusto-macro::interface", "method", "Next", "", "")]]
    auto next(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        if (! endpoint->model.can_next()) co_return Ok(dbusto::Body::make());
        co_return co_await endpoint.get()->action(
            p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_NEXT);
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "Previous", "", "")]]
    auto previous(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        if (! endpoint->model.can_previous()) co_return Ok(dbusto::Body::make());
        co_return co_await endpoint.get()->action(
            p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_PREVIOUS);
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "Pause", "", "")]]
    auto pause(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        if (! endpoint->model.can_pause()) co_return Ok(dbusto::Body::make());
        co_return co_await endpoint.get()->action(
            p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_PAUSE);
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "PlayPause", "", "")]]
    auto play_pause(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        if (! endpoint->model.can_pause())
            co_return Err(dbusto::remote_error("org.freedesktop.DBus.Error.NotSupported"_str,
                                               "no playable track"_str));
        co_return co_await endpoint.get()->action(
            p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_TOGGLE);
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "Stop", "", "")]]
    auto stop(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        return endpoint.get()->action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_STOP);
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "Play", "", "")]]
    auto play(const dbusto::Message&) -> coro<dbusto::Result<dbusto::Body>> {
        if (! endpoint->model.can_play()) co_return Ok(dbusto::Body::make());
        co_return co_await endpoint.get()->action(
            p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_PLAY);
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "Seek", "x", "")]]
    auto seek(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<i64>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        auto command = endpoint->model.seek(args->template get<0>());
        if (command.is_none()) co_return Ok(dbusto::Body::make());
        co_return co_await endpoint.get()->control(rstd::move(*command));
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "SetPosition", "ox", "")]]
    auto set_position(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<dbusto::ObjectPath, i64>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        auto command = endpoint->model.set_position(args->template get<0>().text.as_str(),
                                                    args->template get<1>());
        if (command.is_none()) co_return Ok(dbusto::Body::make());
        co_return co_await endpoint.get()->control(rstd::move(*command));
    }
    [[pmacro::helper("dbusto-macro::interface", "method", "OpenUri", "s", "")]]
    auto open_uri(const dbusto::Message& call) -> coro<dbusto::Result<dbusto::Body>> {
        auto args = dbusto::unpack<String>(call.body);
        if (args.is_err()) co_return Err(rstd::move(args).unwrap_err());
        p_qcm::p_msg::M_PlayerControlReq command;
        command.f_action =
            i32(static_cast<int>(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_SOURCE));
        command.f_source = rstd::move(args->template get<0>());
        co_return co_await endpoint.get()->control(rstd::move(command));
    }
    [[pmacro::helper("dbusto-macro::interface", "signal", "Seeked", "x")]]
    static auto seeked() -> dbusto::SignalSpec;

    [[pmacro::helper("dbusto-macro::interface", "property", "PlaybackStatus", "s", "read", "true")]]
    auto playback_status() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text(String::make(endpoint->model.playback_status())));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "LoopStatus", "s", "readwrite", "true",
                     "set_loop_status")]]
    auto loop_status() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Text(String::make(endpoint->model.loop_status())));
    }
    auto set_loop_status(const dbusto::Value& value) -> coro<dbusto::Result<empty>> {
        auto decoded = dbusto::Codec<String>::decode(value);
        if (decoded.is_err()) co_return Err(rstd::move(decoded).unwrap_err());
        p_qcm::p_msg::M_PlayerQueueReq command;
        command.f_action =
            i32(static_cast<int>(p_qcm::p_msg::E_PlayerQueueAction::v_PLAYER_QUEUE_ACTION_LOOP));
        if (*decoded == "None"_str)
            command.f_loop_mode = i32();
        else if (*decoded == "Track"_str)
            command.f_loop_mode = i32(1);
        else if (*decoded == "Playlist"_str)
            command.f_loop_mode = i32(2);
        else
            co_return Err(dbusto::type_error());
        co_return co_await endpoint.get()->queue(rstd::move(command));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "Rate", "d", "readwrite", "false",
                     "set_rate")]]
    auto rate() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Floating(1.0));
    }
    auto set_rate(const dbusto::Value& value) -> coro<dbusto::Result<empty>> {
        auto decoded = dbusto::Codec<double>::decode(value);
        if (decoded.is_err()) co_return Err(rstd::move(decoded).unwrap_err());
        if (! f64(*decoded).is_finite()) co_return Err(dbusto::type_error());
        if (*decoded == 0.0)
            co_return co_await MprisEndpoint::completed(
                endpoint.get()->action(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_PAUSE));
        co_return Ok(empty {});
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "Shuffle", "b", "readwrite", "true",
                     "set_shuffle")]]
    auto shuffle() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(endpoint->model.shuffle()));
    }
    auto set_shuffle(const dbusto::Value& value) -> coro<dbusto::Result<empty>> {
        auto decoded = dbusto::Codec<bool>::decode(value);
        if (decoded.is_err()) co_return Err(rstd::move(decoded).unwrap_err());
        p_qcm::p_msg::M_PlayerQueueReq command;
        command.f_action =
            i32(static_cast<int>(p_qcm::p_msg::E_PlayerQueueAction::v_PLAYER_QUEUE_ACTION_SHUFFLE));
        command.f_shuffle = Some<bool>(*decoded);
        co_return co_await endpoint.get()->queue(rstd::move(command));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "Metadata", "a{sv}", "read", "true")]]
    auto metadata() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Codec<dbusto::PropertyMap>::encode(endpoint->model.metadata()));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "Volume", "d", "readwrite", "true",
                     "set_volume")]]
    auto volume() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Floating(endpoint->model.volume()));
    }
    auto set_volume(const dbusto::Value& value) -> coro<dbusto::Result<empty>> {
        auto decoded = dbusto::Codec<double>::decode(value);
        if (decoded.is_err()) co_return Err(rstd::move(decoded).unwrap_err());
        if (! f64(*decoded).is_finite()) co_return Err(dbusto::type_error());
        p_qcm::p_msg::M_PlayerControlReq command;
        command.f_action =
            i32(static_cast<int>(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_VOLUME));
        command.f_volume = f64(*decoded < 0.0 ? 0.0 : *decoded);
        co_return co_await MprisEndpoint::completed(endpoint.get()->control(rstd::move(command)));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "Position", "x", "read", "false")]]
    auto position() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Int64(endpoint->model.position()));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "MinimumRate", "d", "read", "const")]]
    auto minimum_rate() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Floating(1.0));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "MaximumRate", "d", "read", "const")]]
    auto maximum_rate() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Floating(1.0));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "CanGoNext", "b", "read", "true")]]
    auto can_next() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(endpoint->model.can_next()));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "CanGoPrevious", "b", "read", "true")]]
    auto can_previous() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(endpoint->model.can_previous()));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "CanPlay", "b", "read", "true")]]
    auto can_play() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(endpoint->model.can_play()));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "CanPause", "b", "read", "true")]]
    auto can_pause() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(endpoint->model.can_pause()));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "CanSeek", "b", "read", "true")]]
    auto can_seek() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(endpoint->model.can_seek()));
    }
    [[pmacro::helper("dbusto-macro::interface", "property", "CanControl", "b", "read", "const")]]
    auto can_control() -> coro<dbusto::Result<dbusto::Value>> {
        co_return Ok(dbusto::Value::Boolean(true));
    }
};

class DesktopMpris {
    Rc<MprisEndpoint>                                endpoint_;
    dbusto::Connection                               connection_;
    dbusto::Object                                   object_;
    Option<AbortOnDropHandle<dbusto::Result<empty>>> driver_;
    Option<AbortOnDropHandle<empty>>                 incoming_;
    rstd::async::Mutex<empty>                        updates_ { empty {} };
    rstd::async::Mutex<empty>                        closing_ { empty {} };

    DesktopMpris(Rc<MprisEndpoint> endpoint, dbusto::Connection connection, dbusto::Object object,
                 AbortOnDropHandle<dbusto::Result<empty>> driver);
    static auto dispatch(Rc<MprisEndpoint> endpoint, dbusto::Connection connection,
                         dbusto::Object object) -> coro<empty>;
    static auto error(dbusto::Error error) -> String;
    auto        publish(MprisChanges changes) -> coro<MprisResult>;

public:
    // The owner and its host stay on the backend's current-thread runtime.
    static auto start(Rc<dyn<PlayerHost>> host) -> coro<Result<Rc<DesktopMpris>, String>>;
    static auto start_on_bus(String address, Rc<dyn<PlayerHost>> host)
        -> coro<Result<Rc<DesktopMpris>, String>>;
    auto update_state(PlayerStateSnapshot state) -> coro<MprisResult>;
    auto update_queue(PlayerQueueSnapshot queue) -> coro<MprisResult>;
    auto update(p_qcm::p_msg::M_PlayerStateMsg state, p_qcm::p_msg::M_PlayerQueueMsg queue)
        -> coro<MprisResult>;
    auto shutdown() -> coro<empty>;
    ~DesktopMpris();
};
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<qcm::backend::PlayerIntegration, Rc<qcm::backend::DesktopMpris>>
    : ImplBase<Rc<qcm::backend::DesktopMpris>> {
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
