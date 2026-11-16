export module qcm.protocol:player;
export import rstd;

using namespace rstd::prelude;

export namespace qcm::player_ipc
{
inline constexpr usize MAX_FRAME_BYTES { 1024 * 1024 };

enum class Action
{
    Source,
    Play,
    Pause,
    Stop,
    Seek,
    Volume,
    Fade,
    Snapshot,
    Shutdown
};
struct Command {
    u32    version { 1 };
    u64    id {}, epoch {};
    Action action { Action::Snapshot };
    String source;
    double value {};
    bool   autoplay { true };

    auto clone() const -> Command {
        return { version, id, epoch, action, source.clone(), value, autoplay };
    }
};

enum class EventKind
{
    Ready,
    State,
    Ack,
    Ended,
    Seeked,
    Error
};
struct State {
    u64    epoch {}, sequence {};
    String source;
    i32    playback_state { 2 };
    i64    position_ms {}, duration_ms {};
    double volume { 1 };
    u32    fade_ms { 500 };
    bool   busy {}, seekable {};
    double cache_begin {}, cache_end {};
    String error;

    auto clone() const -> State {
        return { epoch,       sequence,  source.clone(), playback_state, position_ms,
                 duration_ms, volume,    fade_ms,        busy,           seekable,
                 cache_begin, cache_end, error.clone() };
    }
};
struct Event {
    u32       version { 1 };
    u64       id {};
    EventKind kind { EventKind::State };
    State     state;

    auto clone() const -> Event { return { version, id, kind, state.clone() }; }
};

// Ack means validated and queued, not completed. Seeked reports a completed seek.
// Source requires a strictly newer epoch; source-dependent actions target the current epoch.
// Shutdown is independent of the source epoch.
// Shutdown and stdin EOF cancel work without waiting for the engine queue to drain.
// Each emitted event advances sequence, including snapshots and errors.
// The process reserves id=0 for unsolicited events and fatal transport errors.
// Rejected commands and engine failures use the originating command id.
// Shutdown also accepts id=0 for parent-side teardown without a pending request.
auto encode_command(const Command&) -> Result<String, String>;
auto encode_event(const Event&) -> Result<String, String>;
auto decode_command(ref<str>) -> Result<Command, String>;
auto decode_event(ref<str>) -> Result<Event, String>;

// LF terminates frames; CRLF is accepted. Frames exclude the delimiter, must be
// nonempty UTF-8, and may not exceed MAX_FRAME_BYTES (excluding CRLF).
// Typed decoders validate JSON syntax/schema; framing never builds a second DOM.
// A malformed frame poisons the stream. EOF is valid only at a frame boundary.
class JsonLines {
    Vec<u8> pending_;
    bool    failed_ {}, finished_ {};

public:
    auto feed(slice<u8>) -> Result<Vec<String>, String>;
    auto finish() -> Result<empty, String>;
};
} // namespace qcm::player_ipc
