module;
#include <rstd/macro.hpp>
module qcm.protocol;
import :player;
import rstd.json;
import rstd.serde;
using namespace rstd::literals;

namespace qcm::player_ipc
{
namespace codec
{
enum class [[pmacro::derive("serde::derive"),
             pmacro::helper("serde::derive", string_enum)]] WireAction
{
    Source [[pmacro::helper("serde::derive", rename("source"))]],
    Play [[pmacro::helper("serde::derive", rename("play"))]],
    Pause [[pmacro::helper("serde::derive", rename("pause"))]],
    Stop [[pmacro::helper("serde::derive", rename("stop"))]],
    Seek [[pmacro::helper("serde::derive", rename("seek"))]],
    Volume [[pmacro::helper("serde::derive", rename("volume"))]],
    Fade [[pmacro::helper("serde::derive", rename("fade"))]],
    Snapshot [[pmacro::helper("serde::derive", rename("snapshot"))]],
    Shutdown [[pmacro::helper("serde::derive", rename("shutdown"))]],
};
enum class [[pmacro::derive("serde::derive"),
             pmacro::helper("serde::derive", string_enum)]] WireKind
{
    Ready [[pmacro::helper("serde::derive", rename("ready"))]],
    State [[pmacro::helper("serde::derive", rename("state"))]],
    Ack [[pmacro::helper("serde::derive", rename("ack"))]],
    Ended [[pmacro::helper("serde::derive", rename("ended"))]],
    Seeked [[pmacro::helper("serde::derive", rename("seeked"))]],
    Error [[pmacro::helper("serde::derive", rename("error"))]],
};
struct [[pmacro::derive("serde::derive")]] WireCommand {
    u32                                                 version;
    u64                                                 id, epoch;
    WireAction                                          action;
    [[pmacro::helper("serde::derive", default)]] String source;
    [[pmacro::helper("serde::derive", default)]] f64    value;
    [[pmacro::helper("serde::derive", default)]] bool   autoplay { true };
};
struct [[pmacro::derive("serde::derive")]] WireState {
    u64    epoch, sequence;
    String source;
    i32    playback_state;
    i64    position_ms, duration_ms;
    f64    volume;
    u32    fade_ms;
    bool   busy, seekable;
    f64    cache_begin, cache_end;
    String error;
};
struct [[pmacro::derive("serde::derive")]] WireEvent {
    u32       version;
    u64       id;
    WireKind  kind;
    WireState state;
};

auto wire(const State& value) -> WireState {
    return { value.epoch,        value.sequence,    value.source.clone(),   value.playback_state,
             value.position_ms,  value.duration_ms, f64(value.volume),      value.fade_ms,
             value.busy,         value.seekable,    f64(value.cache_begin), f64(value.cache_end),
             value.error.clone() };
}
auto state(WireState value) -> State {
    return { value.epoch,
             value.sequence,
             rstd::move(value.source),
             value.playback_state,
             value.position_ms,
             value.duration_ms,
             value.volume.to_primitive(),
             value.fade_ms,
             value.busy,
             value.seekable,
             value.cache_begin.to_primitive(),
             value.cache_end.to_primitive(),
             rstd::move(value.error) };
}
auto valid(const Command& value) -> Result<empty, String> {
    if (value.version != u32(1)) return Err("unsupported player protocol version"_Str);
    if (! f64(value.value).is_finite()) return Err("non-finite command value"_Str);
    switch (value.action) {
    case Action::Source:
    case Action::Play:
    case Action::Pause:
    case Action::Stop:
    case Action::Snapshot:
    case Action::Shutdown:
    case Action::Seek:
    case Action::Volume:
    case Action::Fade: break;
    default: return Err("invalid player action"_Str);
    }
    return Ok(empty {});
}
auto valid(const Event& value) -> Result<empty, String> {
    if (value.version != u32(1)) return Err("unsupported player protocol version"_Str);
    if (value.state.playback_state < i32() || value.state.playback_state > i32(2) ||
        value.state.position_ms < i64() || value.state.duration_ms < i64() ||
        ! f64(value.state.volume).is_finite() || value.state.volume < 0 || value.state.volume > 1 ||
        ! f64(value.state.cache_begin).is_finite() || ! f64(value.state.cache_end).is_finite() ||
        value.state.cache_begin < 0 || value.state.cache_end > 1 ||
        value.state.cache_begin > value.state.cache_end)
        return Err("invalid player state"_Str);
    switch (value.kind) {
    case EventKind::Ready:
    case EventKind::State:
    case EventKind::Ack:
    case EventKind::Ended:
    case EventKind::Seeked:
    case EventKind::Error: return Ok(empty {});
    }
    return Err("invalid player event kind"_Str);
}
template<class T>
auto encode(const T& value) -> Result<String, String> {
    auto result = rstd_try(rstd::json::encode(value), [](auto error) {
        return rstd::format("invalid player message: {}", error);
    });
    if (result.len() > MAX_FRAME_BYTES) return Err("player frame too large"_Str);
    result.push(U'\n');
    return Ok(rstd::move(result));
}
template<class T>
auto decode(ref<str> input) -> Result<T, String> {
    if (input.ends_with("\n"_str)) {
        input = input.get(usize(), input.len() - usize(1)).unwrap();
        if (input.ends_with("\r"_str)) input = input.get(usize(), input.len() - usize(1)).unwrap();
    }
    if (input.len() > MAX_FRAME_BYTES) return Err("player frame too large"_Str);
    return rstd::json::decode_direct<T>(input).map_err([](auto error) {
        return rstd::format("invalid player message: {}", error);
    });
}
} // namespace codec

using namespace codec;

auto encode_command(const Command& value) -> Result<String, String> {
    rstd_try(valid(value));
    return encode(WireCommand { value.version,
                                value.id,
                                value.epoch,
                                static_cast<WireAction>(value.action),
                                value.source.clone(),
                                f64(value.value),
                                value.autoplay });
}
auto decode_command(ref<str> input) -> Result<Command, String> {
    auto    value = rstd_try(decode<WireCommand>(input));
    Command result { value.version,
                     value.id,
                     value.epoch,
                     static_cast<Action>(value.action),
                     rstd::move(value.source),
                     value.value.to_primitive(),
                     value.autoplay };
    rstd_try(valid(result));
    return Ok(rstd::move(result));
}
auto encode_event(const Event& value) -> Result<String, String> {
    rstd_try(valid(value));
    return encode(WireEvent {
        value.version, value.id, static_cast<WireKind>(value.kind), wire(value.state) });
}
auto decode_event(ref<str> input) -> Result<Event, String> {
    auto  value = rstd_try(decode<WireEvent>(input));
    Event result {
        value.version, value.id, static_cast<EventKind>(value.kind), state(rstd::move(value.state))
    };
    rstd_try(valid(result));
    return Ok(rstd::move(result));
}

auto JsonLines::feed(slice<u8> input) -> Result<Vec<String>, String> {
    if (failed_ || finished_) return Err("player stream is closed"_Str);
    Vec<String> lines;
    auto        fail = [&](ref<str> error) -> Result<Vec<String>, String> {
        pending_.clear();
        failed_ = true;
        return Err(String::make(error));
    };
    for (auto byte : input) {
        if (byte != u8('\n')) {
            if (pending_.len() >= MAX_FRAME_BYTES + usize(1))
                return fail("player frame too large"_str);
            pending_.push(u8(byte));
            if (pending_.len() > MAX_FRAME_BYTES && byte != u8('\r'))
                return fail("player frame too large"_str);
            continue;
        }
        if (! pending_.is_empty() && pending_[pending_.len() - usize(1)] == u8('\r'))
            (void)pending_.pop();
        if (pending_.is_empty()) return fail("empty player frame"_str);
        auto text = String::from_utf8(rstd::move(pending_));
        pending_  = Vec<u8>();
        if (text.is_err()) return fail("invalid UTF-8 player frame"_str);
        lines.push(rstd::move(text).unwrap());
    }
    return Ok(rstd::move(lines));
}
auto JsonLines::finish() -> Result<empty, String> {
    if (failed_ || finished_) return Err("player stream is closed"_Str);
    finished_ = true;
    if (! pending_.is_empty()) {
        pending_.clear();
        failed_ = true;
        return Err("truncated player frame at EOF"_Str);
    }
    return Ok(empty {});
}
} // namespace qcm::player_ipc
