module;
#include <rstd/enum.hpp>
export module qcm.player.bin:process;
import qcm.player;
import qcm.player.io;
import qcm.player.protocol;
import :transport;
import rstd;

using alloc::collections::VecDeque;
using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using rstd::async::Notify;
using rstd::sync::Arc;
using rstd::sync::Mutex;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace ipc = qcm::player_ipc;

namespace qcm::player_process
{
struct Cache {
    double begin {}, end {};
};
struct Delta {
    Option<i64>   position, duration;
    Option<i32>   playback;
    Option<bool>  busy, seekable;
    Option<Cache> cache;
};
struct Notification {
    u64            generation {};
    ipc::EventKind kind { ipc::EventKind::State };
    u64            id {};
    Delta          delta;
    String         error;
};
class NotifyChannel final {
    struct Pending {
        Vec<Notification> items;
        u64               generation {};
        bool              overflow {};

        auto append(ipc::EventKind kind, u64 id = {}) -> Notification* {
            if (items.len() >= usize(256)) {
                overflow = true;
                return nullptr;
            }
            items.push(Notification { generation, kind, id, {}, {} });
            return &*items.last_mut().unwrap();
        }
    };
    Mutex<Pending> pending_;
    Notify         changed_;

public:
    struct Batch {
        Vec<Notification> items;
        u64               generation;
        bool              overflow;
    };
    explicit NotifyChannel(Notify changed): changed_(rstd::move(changed)) {}

    auto advance_epoch() const -> rstd::uint64_t {
        auto lock = pending_.lock().unwrap();
        lock->items.clear();
        return (++lock->generation).to_primitive();
    }
    bool send(player::notify::info info) const {
        bool accepted {};
        {
            auto  lock = pending_.lock().unwrap();
            auto* item = info.is_ended() ? lock->append(ipc::EventKind::Ended)
                         : ! lock->items.is_empty() &&
                                 lock->items.last().unwrap()->kind == ipc::EventKind::State
                             ? &*lock->items.last_mut().unwrap()
                             : lock->append(ipc::EventKind::State);
            accepted   = item != nullptr;
            if (item) {
                auto& delta = item->delta;
                RSTD_MATCH(info) {
                    RSTD_CASE(position, value) { delta.position = Some(i64(value)); }
                    RSTD_CASE(duration, value) { delta.duration = Some(i64(value)); }
                    RSTD_CASE(playstate, value) {
                        delta.playback = Some(i32(static_cast<int>(value)));
                    }
                    RSTD_CASE(busy, value) { delta.busy = Some(value); }
                    RSTD_CASE(cache, begin, end) { delta.cache = Some(Cache { begin, end }); }
                    RSTD_CASE(seekable, value) { delta.seekable = Some(value); }
                    RSTD_CASE(ended) {}
                }
            }
        }
        command_consumed();
        return accepted;
    }
    void error(rstd::uint64_t id, ref<str> text) const {
        {
            auto lock = pending_.lock().unwrap();
            if (auto* item = lock->append(ipc::EventKind::Error, u64(id)))
                item->error = String::make(text);
        }
        command_consumed();
    }
    void seeked(rstd::uint64_t id, rstd::int64_t position) const {
        {
            auto lock = pending_.lock().unwrap();
            if (auto* item = lock->append(ipc::EventKind::Seeked, u64(id)))
                item->delta.position = Some(i64(position));
        }
        command_consumed();
    }
    void command_consumed() const { (void)changed_.notifier().notify(); }
    auto notified() const -> rstd::async::NotifyFuture { return changed_.notified(); }
    auto take() -> Batch {
        auto lock = pending_.lock().unwrap();
        return { rstd::exchange(lock->items, Vec<Notification> {}),
                 lock->generation,
                 lock->overflow };
    }
};
} // namespace qcm::player_process

namespace rstd
{
template<>
struct Impl<player::detail::NotifySink, Arc<qcm::player_process::NotifyChannel>>
    : ImplBase<Arc<qcm::player_process::NotifyChannel>> {
    auto send(player::notify::info value) const -> bool {
        return this->self()->send(rstd::move(value));
    }
    auto advance_epoch() const -> rstd::uint64_t { return this->self()->advance_epoch(); }
    void error(rstd::uint64_t id, ref<str> text) const { this->self()->error(id, text); }
    void seeked(rstd::uint64_t id, rstd::int64_t position) const {
        this->self()->seeked(id, position);
    }
    void command_consumed() const { this->self()->command_consumed(); }
};
} // namespace rstd

namespace qcm::player_process
{

class Session {
    Arc<NotifyChannel>          channel_;
    player::Player              engine_;
    rstd::async::pipe::Receiver input_pipe_;
    PipeWriter                  writer_;
    ipc::JsonLines              input_;
    ipc::State                  state_;
    VecDeque<ipc::Command>      pending_;
    usize                       pending_bytes_ {};
    bool                        stopping_ {};
    i32                         result_ {};

    void shutdown(i32 result = {}) {
        if (result > result_) result_ = result;
        if (stopping_) return;
        stopping_ = true;
        pending_.clear();
        pending_bytes_ = usize();
        engine_.close();
    }
    void publish(ipc::EventKind kind, u64 id = {}, ref<str> error = ""_str) {
        ++state_.sequence;
        auto event = ipc::Event { u32(1), id, kind, state_.clone() };
        if (! error.is_empty()) event.state.error = String::make(error);
        auto encoded = ipc::encode_event(event);
        if (encoded.is_err() ||
            ! writer_.push(encoded->as_str(), kind == ipc::EventKind::State && id == u64())) {
            rstd::io::eprintln("qcm-player: output queue unavailable");
            shutdown(i32(1));
        }
    }
    void error(u64 id, ref<str> message) { publish(ipc::EventKind::Error, id, message); }
    void notifications() {
        auto batch = channel_->take();
        for (auto& item : batch.items) {
            if (item.generation != batch.generation) continue;
            auto& delta = item.delta;
            if (delta.position.is_some()) state_.position_ms = delta.position->max(i64());
            if (delta.duration.is_some()) state_.duration_ms = delta.duration->max(i64());
            if (delta.playback.is_some()) state_.playback_state = *delta.playback;
            if (delta.busy.is_some()) state_.busy = *delta.busy;
            if (delta.seekable.is_some()) state_.seekable = *delta.seekable;
            if (delta.cache.is_some()) {
                state_.cache_begin =
                    rstd::f64(delta.cache->begin).clamp(rstd::f64(), rstd::f64(1)).to_primitive();
                state_.cache_end = rstd::f64(delta.cache->end)
                                       .clamp(rstd::f64(state_.cache_begin), rstd::f64(1))
                                       .to_primitive();
            }
            if (item.kind == ipc::EventKind::Error) {
                rstd::io::eprintln("qcm-player: command {}: {}", item.id, item.error);
                state_.error = rstd::move(item.error);
                state_.busy  = false;
            }
            publish(item.kind, item.id);
            if (item.kind == ipc::EventKind::Error) publish(ipc::EventKind::State);
        }
        if (batch.overflow) {
            error({}, "engine notification queue overflow"_str);
            shutdown(i32(1));
        }
    }
    void command(ipc::Command command) {
        if (command.id == u64() && command.action != ipc::Action::Shutdown) {
            error({}, "command id zero is reserved for transport errors"_str);
            shutdown(i32(1));
            return;
        }
        if (command.action != ipc::Action::Shutdown &&
            (command.action == ipc::Action::Source ? command.epoch <= state_.epoch
                                                   : command.epoch != state_.epoch)) {
            error(command.id, "stale or invalid source epoch"_str);
            return;
        }
        if ((command.action == ipc::Action::Seek &&
             (command.value < 0 || command.value >= 9223372036854775808.0)) ||
            (command.action == ipc::Action::Volume && (command.value < 0 || command.value > 1)) ||
            (command.action == ipc::Action::Fade &&
             (command.value < 0 || command.value > 4294967295.0 ||
              rstd::f64(command.value).floor() != rstd::f64(command.value)))) {
            error(command.id, "command value out of range"_str);
            return;
        }
        const auto id = command.id.to_primitive();
        switch (command.action) {
        case ipc::Action::Source: {
            auto source        = parse_source(command.source.as_str());
            state_.epoch       = command.epoch;
            state_.source      = rstd::move(command.source);
            state_.position_ms = state_.duration_ms = i64();
            state_.playback_state                   = i32(2);
            state_.seekable                         = false;
            state_.cache_begin                      = 0;
            state_.cache_end =
                source.is_ok() && source->local && ! state_.source.is_empty() ? 1 : 0;
            state_.error.clear();
            state_.busy = true;
            if (source.is_err()) {
                engine_.stop(id);
                state_.busy  = false;
                state_.error = rstd::move(source).unwrap_err();
                publish(ipc::EventKind::Error, command.id);
                publish(ipc::EventKind::State);
                return;
            }
            engine_.set_source(source->uri.as_str(), rstd::move(source->io), id, command.autoplay);
            break;
        }
        case ipc::Action::Play:
            state_.busy = true;
            engine_.play(id);
            break;
        case ipc::Action::Pause:
            state_.busy = true;
            engine_.pause(id);
            break;
        case ipc::Action::Stop:
            state_.source.clear();
            state_.position_ms = state_.duration_ms = i64();
            state_.seekable                         = false;
            state_.error.clear();
            state_.cache_begin = state_.cache_end = 0;
            state_.busy                           = true;
            engine_.stop(id);
            break;
        case ipc::Action::Seek:
            state_.busy = true;
            engine_.seek(static_cast<rstd::int64_t>(command.value), id);
            break;
        case ipc::Action::Volume:
            engine_.set_volume(float(command.value), id);
            state_.volume = engine_.volume();
            break;
        case ipc::Action::Fade:
            state_.fade_ms = u32(static_cast<rstd::uint32_t>(command.value));
            engine_.set_fade_time_ms(state_.fade_ms.to_primitive());
            break;
        case ipc::Action::Snapshot: break;
        case ipc::Action::Shutdown: break;
        }
        publish(ipc::EventKind::Ack, command.id);
        if (command.action == ipc::Action::Snapshot)
            publish(ipc::EventKind::State, command.id);
        else if (command.action != ipc::Action::Shutdown)
            publish(ipc::EventKind::State);
        if (command.action == ipc::Action::Shutdown) shutdown();
    }
    void dispatch() {
        while (! stopping_ && engine_.pending_actions() < 64) {
            auto next = pending_.pop_front();
            if (next.is_none()) break;
            pending_bytes_ -= next->source.len() + usize(sizeof(ipc::Command));
            command(rstd::move(next).unwrap());
        }
    }
    void read(rstd::slice<rstd::u8> bytes) {
        if (bytes.is_empty()) {
            auto complete = input_.finish();
            if (complete.is_err()) error({}, complete.unwrap_err().as_str());
            shutdown(complete.is_err() ? i32(1) : i32());
            return;
        }
        auto decoded = input_.feed(bytes);
        if (decoded.is_err()) {
            error({}, decoded.unwrap_err().as_str());
            shutdown(i32(1));
            return;
        }
        for (auto& line : *decoded) {
            auto next = ipc::decode_command(line.as_str());
            if (next.is_err()) {
                error({}, next.unwrap_err().as_str());
                shutdown(i32(1));
                return;
            }
            if (next->action == ipc::Action::Shutdown) {
                dispatch();
                if (! stopping_) command(rstd::move(*next));
                return;
            }
            const auto size = next->source.len() + usize(sizeof(ipc::Command));
            if (pending_.len() >= usize(128) || pending_bytes_ + size > usize(2 * 1024 * 1024)) {
                error({}, "command queue overflow"_str);
                shutdown(i32(1));
                return;
            }
            pending_bytes_ += size;
            pending_.push_back(rstd::move(*next));
        }
    }

public:
    Session(Notify changed, rstd::async::pipe::Receiver input, PipeWriter writer)
        : channel_(Arc<NotifyChannel>::make(rstd::move(changed))),
          engine_("Qcm"_str,
                  player::Notifier(Arc<dyn<player::detail::NotifySink>>::make(channel_.clone()))),
          input_pipe_(rstd::move(input)),
          writer_(rstd::move(writer)) {}

    auto run() -> coro<i32> {
        auto engine = AbortOnDropHandle<void>(rstd::async::spawn_local(engine_.process_actions()));
        publish(ipc::EventKind::Ready);
        while (! stopping_) {
            notifications();
            if (writer_.failed()) shutdown(i32(1));
            dispatch();
            if (stopping_) break;
            auto incoming = co_await rstd::async::select(
                channel_->notified(), input_pipe_.read(usize(16 * 1024)).into_future());
            if (incoming.is_left()) {
                if (rstd::move(incoming).unwrap_left().is_err()) shutdown(i32(1));
            } else {
                auto bytes = rstd::move(incoming).unwrap_right();
                if (bytes.is_err()) {
                    error({}, "stdin read failed"_str);
                    shutdown(i32(1));
                } else {
                    read(bytes->as_slice());
                }
            }
        }
        engine_.close();
        (void)co_await rstd::move(input_pipe_).close();
        if ((co_await rstd::move(engine)).is_err()) result_ = i32(1);
        notifications();
        state_.busy           = false;
        state_.playback_state = i32(2);
        state_.seekable       = false;
        publish(ipc::EventKind::State);
        if (! co_await writer_.finish()) result_ = i32(1);
        co_return result_;
    }
};

export auto run() -> coro<Result<i32, String>> {
    auto input = rstd::async::pipe::Receiver::stdin();
    if (input.is_err()) co_return Err(rstd::format("stdin pipe: {}", input.unwrap_err()));
    auto output = rstd::async::pipe::Sender::stdout();
    if (output.is_err()) {
        (void)co_await rstd::move(input).unwrap().close();
        co_return Err(rstd::format("stdout pipe: {}", output.unwrap_err()));
    }
    auto changed = Notify::make();
    if (changed.is_err()) {
        (void)co_await rstd::move(input).unwrap().close();
        (void)co_await rstd::move(output).unwrap().close();
        co_return Err(rstd::format("session notification: {}", changed.unwrap_err()));
    }
    auto writer = PipeWriter::make(rstd::move(output).unwrap(), changed->notifier());
    if (writer.is_err()) {
        (void)co_await rstd::move(input).unwrap().close();
        co_return Err(rstd::move(writer).unwrap_err());
    }
    Session session(
        rstd::move(changed).unwrap(), rstd::move(input).unwrap(), rstd::move(writer).unwrap());
    co_return Ok(co_await session.run());
}
} // namespace qcm::player_process
