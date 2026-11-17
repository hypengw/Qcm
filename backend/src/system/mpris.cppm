module;
#include <rstd/macro.hpp>
export module qcm.backend.system.mpris;
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

    static auto microseconds(i64 milliseconds) -> i64 {
        return milliseconds.max(i64()).saturating_mul(i64(1000));
    }
    static auto metadata_changed(const p_qcm::p_msg::M_PlayerStateMsg& previous,
                                 const p_qcm::p_msg::M_PlayerStateMsg& next) -> bool {
        return previous.f_source_epoch != next.f_source_epoch ||
               previous.f_source != next.f_source || previous.f_duration_ms != next.f_duration_ms ||
               previous.f_title != next.f_title || previous.f_album != next.f_album ||
               previous.f_artists.as_slice() != next.f_artists.as_slice() ||
               previous.f_album_artists.as_slice() != next.f_album_artists.as_slice() ||
               previous.f_art_url != next.f_art_url ||
               previous.f_song_id.is_some() != next.f_song_id.is_some();
    }

public:
    auto has_track() const -> bool {
        return ! state_->f_source.is_empty() || state_->f_song_id.is_some();
    }
    auto connected() const -> bool { return state_->f_connected; }
    auto source_epoch() const -> u64 { return state_->f_source_epoch; }
    auto can_play() const -> bool { return connected() && (has_track() || queued_tracks_); }
    auto can_pause() const -> bool { return can_play(); }
    auto can_seek() const -> bool { return connected() && has_track() && state_->f_seekable; }
    auto can_next() const -> bool { return connected() && can_next_; }
    auto can_previous() const -> bool { return connected() && can_previous_; }
    auto shuffle() const -> bool { return shuffle_; }
    auto volume() const -> double { return state_->f_volume.to_primitive(); }
    auto duration() const -> i64 { return microseconds(state_->f_duration_ms); }
    auto position() const -> i64 {
        auto value = microseconds(state_->f_position_ms);
        return state_->f_duration_ms > i64() ? value.min(duration()) : value;
    }
    auto track_id() const -> String {
        if (! has_track()) return String::make(mpris_no_track);
        return rstd::format("/io/github/hypengw/Qcm/track/e{}", state_->f_source_epoch);
    }
    auto playback_status() const -> ref<str> {
        if (! connected() || ! has_track()) return "Stopped"_str;
        if (state_->f_playback_state == i32()) return "Playing"_str;
        if (state_->f_playback_state == i32(1)) return "Paused"_str;
        return "Stopped"_str;
    }
    auto loop_status() const -> ref<str> {
        if (loop_mode_ == i32(1)) return "Track"_str;
        if (loop_mode_ == i32(2)) return "Playlist"_str;
        return "None"_str;
    }
    auto metadata() const -> dbusto::PropertyMap {
        auto result = dbusto::PropertyMap::make();
        result.insert(String::make("mpris:trackid"_str), dbusto::Value::Path(track_id()));
        if (! has_track()) return result;
        if (state_->f_duration_ms > i64())
            result.insert(String::make("mpris:length"_str), dbusto::Value::Int64(duration()));
        if (! state_->f_title.is_empty())
            result.insert(String::make("xesam:title"_str),
                          dbusto::Value::Text(state_->f_title.clone()));
        if (! state_->f_album.is_empty())
            result.insert(String::make("xesam:album"_str),
                          dbusto::Value::Text(state_->f_album.clone()));
        if (! state_->f_artists.is_empty())
            result.insert(String::make("xesam:artist"_str),
                          dbusto::Codec<Vec<String>>::encode(state_->f_artists.clone()));
        if (! state_->f_album_artists.is_empty())
            result.insert(String::make("xesam:albumArtist"_str),
                          dbusto::Codec<Vec<String>>::encode(state_->f_album_artists.clone()));
        if (! state_->f_art_url.is_empty())
            result.insert(String::make("mpris:artUrl"_str),
                          dbusto::Value::Text(state_->f_art_url.clone()));
        if (! state_->f_source.is_empty())
            result.insert(String::make("xesam:url"_str),
                          dbusto::Value::Text(state_->f_source.clone()));
        return result;
    }
    auto update_state(PlayerStateSnapshot snapshot) -> MprisChanges {
        const auto&  state = *snapshot;
        MprisChanges changes;
        if (received_state_ && (state.f_revision < state_->f_revision ||
                                state.f_source_epoch < state_->f_source_epoch))
            return changes;
        const auto status           = playback_status();
        const auto play             = can_play();
        const auto pause            = can_pause();
        const auto seek             = can_seek();
        const auto next             = can_next();
        const auto previous         = can_previous();
        const auto old_volume       = volume();
        const auto changed_metadata = metadata_changed(*state_, state);
        const auto sought = received_state_ && state.f_source_epoch == state_->f_source_epoch &&
                            state.f_seek_serial > state_->f_seek_serial;
        state_            = rstd::move(snapshot);
        received_state_   = true;
        if (sought) changes.seeked = Some(position());
        if (status != playback_status())
            changes.properties.insert(String::make("PlaybackStatus"_str),
                                      dbusto::Value::Text(String::make(playback_status())));
        if (play != can_play())
            changes.properties.insert(String::make("CanPlay"_str),
                                      dbusto::Value::Boolean(can_play()));
        if (pause != can_pause())
            changes.properties.insert(String::make("CanPause"_str),
                                      dbusto::Value::Boolean(can_pause()));
        if (seek != can_seek())
            changes.properties.insert(String::make("CanSeek"_str),
                                      dbusto::Value::Boolean(can_seek()));
        if (next != can_next())
            changes.properties.insert(String::make("CanGoNext"_str),
                                      dbusto::Value::Boolean(can_next()));
        if (previous != can_previous())
            changes.properties.insert(String::make("CanGoPrevious"_str),
                                      dbusto::Value::Boolean(can_previous()));
        if (old_volume != volume())
            changes.properties.insert(String::make("Volume"_str),
                                      dbusto::Value::Floating(volume()));
        if (changed_metadata)
            changes.properties.insert(String::make("Metadata"_str),
                                      dbusto::Codec<dbusto::PropertyMap>::encode(metadata()));
        return changes;
    }
    auto update_state(p_qcm::p_msg::M_PlayerStateMsg state) -> MprisChanges {
        return update_state(PlayerStateSnapshot::make(rstd::move(state)));
    }
    auto update_queue(const p_qcm::p_msg::M_PlayerQueueMsg& queue) -> MprisChanges {
        MprisChanges changes;
        if (received_queue_ && queue.f_revision < queue_revision_) return changes;
        const auto loop     = loop_status();
        const auto shuffled = shuffle_;
        const auto next     = can_next();
        const auto previous = can_previous();
        const auto play     = can_play();
        queue_revision_     = queue.f_revision;
        loop_mode_          = queue.f_loop_mode;
        shuffle_            = queue.f_shuffle;
        can_next_           = queue.f_can_next;
        can_previous_       = queue.f_can_previous;
        queued_tracks_      = ! queue.f_song_ids.is_empty();
        received_queue_     = true;
        if (loop != loop_status())
            changes.properties.insert(String::make("LoopStatus"_str),
                                      dbusto::Value::Text(String::make(loop_status())));
        if (shuffled != shuffle_)
            changes.properties.insert(String::make("Shuffle"_str),
                                      dbusto::Value::Boolean(shuffle_));
        if (next != can_next())
            changes.properties.insert(String::make("CanGoNext"_str),
                                      dbusto::Value::Boolean(can_next()));
        if (previous != can_previous())
            changes.properties.insert(String::make("CanGoPrevious"_str),
                                      dbusto::Value::Boolean(can_previous()));
        if (play != can_play()) {
            changes.properties.insert(String::make("CanPlay"_str),
                                      dbusto::Value::Boolean(can_play()));
            changes.properties.insert(String::make("CanPause"_str),
                                      dbusto::Value::Boolean(can_pause()));
        }
        return changes;
    }
    auto seek(i64 offset) const -> Option<p_qcm::p_msg::M_PlayerControlReq> {
        if (! can_seek()) return None();
        const auto                       target = position().saturating_add(offset).max(i64());
        p_qcm::p_msg::M_PlayerControlReq command;
        command.f_expected_source_epoch = source_epoch();
        if (duration() > i64() && target > duration()) {
            if (! can_next()) return None();
            command.f_action =
                i32(static_cast<int>(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_NEXT));
        } else {
            command.f_action =
                i32(static_cast<int>(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_SEEK));
            command.f_position_ms = target / i64(1000);
        }
        return Some(rstd::move(command));
    }
    auto set_position(ref<str> track, i64 position) const
        -> Option<p_qcm::p_msg::M_PlayerControlReq> {
        if (! can_seek() || track == mpris_no_track || track != track_id().as_str() ||
            position < i64() || (duration() > i64() && position > duration()))
            return None();
        p_qcm::p_msg::M_PlayerControlReq command;
        command.f_action =
            i32(static_cast<int>(p_qcm::p_msg::E_PlayerAction::v_PLAYER_ACTION_SEEK));
        command.f_position_ms           = position / i64(1000);
        command.f_expected_source_epoch = source_epoch();
        return Some(rstd::move(command));
    }
};

struct MprisEndpoint {
    Rc<dyn<PlayerHost>> host;
    MprisModel          model;
    bool                stopped {};
    bool                quit_requested {};

    explicit MprisEndpoint(Rc<dyn<PlayerHost>> host): host(rstd::move(host)) {}
    static auto failed(String reason) -> dbusto::Error {
        return dbusto::Error::Remote(String::make("org.freedesktop.DBus.Error.Failed"_str),
                                     rstd::move(reason));
    }
    auto control(p_qcm::p_msg::M_PlayerControlReq command) -> coro<dbusto::Result<dbusto::Body>> {
        if (stopped) co_return Err(failed(String::make("MPRIS is stopping"_str)));
        auto result = co_await host->control(rstd::move(command));
        if (result.is_err()) co_return Err(failed(rstd::move(result).unwrap_err()));
        co_return Ok(dbusto::Body::make());
    }
    auto action(p_qcm::p_msg::E_PlayerAction action) -> coro<dbusto::Result<dbusto::Body>> {
        p_qcm::p_msg::M_PlayerControlReq command;
        command.f_action = i32(static_cast<int>(action));
        return control(rstd::move(command));
    }
    auto queue(p_qcm::p_msg::M_PlayerQueueReq command) -> coro<dbusto::Result<empty>> {
        if (stopped) co_return Err(failed(String::make("MPRIS is stopping"_str)));
        auto result = co_await host->queue(rstd::move(command));
        if (result.is_err()) co_return Err(failed(rstd::move(result).unwrap_err()));
        co_return Ok(empty {});
    }
    auto raise() -> coro<dbusto::Result<dbusto::Body>> {
        if (stopped) co_return Err(failed(String::make("MPRIS is stopping"_str)));
        auto result = co_await host->raise();
        if (result.is_err()) co_return Err(failed(rstd::move(result).unwrap_err()));
        co_return Ok(dbusto::Body::make());
    }
    auto after_reply() -> coro<MprisResult> {
        if (! rstd::exchange(quit_requested, false) || stopped) co_return Ok(empty {});
        co_return co_await host->quit();
    }
    static auto completed(coro<dbusto::Result<dbusto::Body>> command)
        -> coro<dbusto::Result<empty>> {
        auto result = co_await rstd::move(command);
        if (result.is_err()) co_return Err(rstd::move(result).unwrap_err());
        co_return Ok(empty {});
    }
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
                 AbortOnDropHandle<dbusto::Result<empty>> driver)
        : endpoint_(rstd::move(endpoint)),
          connection_(rstd::move(connection)),
          object_(rstd::move(object)),
          driver_(Some(rstd::move(driver))) {}
    static auto dispatch(Rc<MprisEndpoint> endpoint, dbusto::Connection connection,
                         dbusto::Object object) -> coro<empty> {
        while (! endpoint->stopped) {
            auto call = co_await connection.next();
            if (call.is_err()) break;
            if (call->type != dbusto::MessageType::MethodCall) continue;
            if ((co_await object.handle(connection.clone(), rstd::move(*call))).is_err()) break;
            if ((co_await endpoint.get()->after_reply()).is_err()) break;
        }
        endpoint.get()->stopped = true;
        (void)co_await connection.close();
        co_return empty {};
    }
    static auto error(dbusto::Error error) -> String { return rstd::format("MPRIS: {}", error); }
    auto        publish(MprisChanges changes) -> coro<MprisResult> {
        if (! changes.properties.is_empty()) {
            auto message = rstd_co_try(
                object_.properties_changed(mpris_player_interface, rstd::move(changes.properties)),
                error);
            rstd_co_try(co_await connection_.send(rstd::move(message)), error);
        }
        if (changes.seeked.is_some()) {
            auto message = rstd_co_try(dbusto::signal(mpris_object_path,
                                                      mpris_player_interface,
                                                      MprisPlayerInterface::seeked(),
                                                      dbusto::pack(*changes.seeked)),
                                       error);
            rstd_co_try(co_await connection_.send(rstd::move(message)), error);
        }
        co_return Ok(empty {});
    }

public:
    // The owner and its host stay on the backend's current-thread runtime.
    static auto start(Rc<dyn<PlayerHost>> host) -> coro<Result<Rc<DesktopMpris>, String>> {
        auto address = rstd::env::var("DBUS_SESSION_BUS_ADDRESS"_str);
        if (address.is_err() || address->is_empty())
            co_return Err(String::make("MPRIS: session bus address is unavailable"_str));
        co_return co_await start_on_bus(rstd::move(*address), rstd::move(host));
    }
    static auto start_on_bus(String address, Rc<dyn<PlayerHost>> host)
        -> coro<Result<Rc<DesktopMpris>, String>> {
        auto endpoint = make_rc<MprisEndpoint>(rstd::move(host));
        auto object   = rstd_co_try(dbusto::Object::make(mpris_object_path), error);
        rstd_co_try(object.add(MprisRootInterface { endpoint.clone() }), error);
        rstd_co_try(object.add(MprisPlayerInterface { endpoint.clone() }), error);
        auto opened     = rstd_co_try(co_await dbusto::connect(address.as_str()), error);
        auto connection = rstd::move(opened.connection);
        auto driver     = AbortOnDropHandle<dbusto::Result<empty>> { rstd::async::spawn_local(
            rstd::move(opened.driver).run()) };
        auto named      = co_await connection.request_name(mpris_bus_name);
        if (named.is_err()) {
            (void)co_await connection.close();
            (void)co_await rstd::move(driver);
            co_return Err(error(rstd::move(named).unwrap_err()));
        }
        auto owner             = Rc<DesktopMpris>(new DesktopMpris(
            endpoint.clone(), rstd::move(connection), rstd::move(object), rstd::move(driver)));
        owner.get()->incoming_ = Some(AbortOnDropHandle<empty> { rstd::async::spawn_local(
            dispatch(rstd::move(endpoint), owner->connection_.clone(), owner->object_.clone())) });
        co_return Ok(rstd::move(owner));
    }
    auto update_state(PlayerStateSnapshot state) -> coro<MprisResult> {
        auto guard = co_await updates_.lock();
        if (endpoint_->stopped) co_return Err(String::make("MPRIS is stopping"_str));
        co_return co_await publish(endpoint_.get()->model.update_state(rstd::move(state)));
    }
    auto update_queue(PlayerQueueSnapshot queue) -> coro<MprisResult> {
        auto guard = co_await updates_.lock();
        if (endpoint_->stopped) co_return Err(String::make("MPRIS is stopping"_str));
        co_return co_await publish(endpoint_.get()->model.update_queue(*queue));
    }
    auto update(p_qcm::p_msg::M_PlayerStateMsg state, p_qcm::p_msg::M_PlayerQueueMsg queue)
        -> coro<MprisResult> {
        rstd_co_try(co_await update_state(PlayerStateSnapshot::make(rstd::move(state))));
        co_return co_await update_queue(PlayerQueueSnapshot::make(rstd::move(queue)));
    }
    auto shutdown() -> coro<empty> {
        auto guard               = co_await closing_.lock();
        endpoint_.get()->stopped = true;
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
    ~DesktopMpris() { endpoint_.get()->stopped = true; }
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
