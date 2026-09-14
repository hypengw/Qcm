module;
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>

#include "core/macro.h"
#include "pcm_queue.h"

module qcm.player;
import :player;
import qcm.core;

import rstd;
import rstd.cppstd;
import wavsen.audio;

using namespace player;
namespace wa = wavsen::audio;
namespace rn = rstd;
using namespace std::chrono_literals;
using namespace rstd::literals;

namespace
{
enum class CommandKind
{
    Source,
    Play,
    Pause,
    Stop,
    Seek,
    Volume
};
struct Command {
    CommandKind   kind;
    std::string   source;
    double        value {};
    std::uint64_t revision {}, source_revision {}, id {};
};
class PcmChannel : public wa::IPullChannel {
public:
    PcmChannel(SpscQueue<PcmBlock, 32>& pcm, SpscQueue<ClockSpan, 512>& clock)
        : m_pcm(pcm), m_clock(clock) {}
    void pass_desc(const wa::DeviceDesc& desc) override {
        m_valid = desc.channels == rn::u32(2) && desc.sample_rate == rn::u32(48000);
    }
    void output_offset(rn::u64 value) override { m_output = value.to_primitive(); }
    auto next_pcm(void* data, rn::u32 requested) -> rn::u64 override {
        if (! m_valid) return rn::u64();
        unsigned produced = 0;
        while (produced < requested.to_primitive()) {
            const auto* block = m_pcm.front();
            if (! block || m_clock.full()) break;
            const auto count =
                std::min(block->frames - m_offset, requested.to_primitive() - produced);
            std::memcpy(static_cast<float*>(data) + produced * 2,
                        block->samples.data() + m_offset * 2,
                        count * 2 * sizeof(float));
            m_clock.push({ m_output + produced, count, block->seconds + double(m_offset) / 48000 });
            produced += count;
            m_offset += count;
            if (m_offset == block->frames) {
                m_offset = 0;
                m_pcm.pop();
            }
        }
        return rn::u64(produced);
    }

private:
    SpscQueue<PcmBlock, 32>&   m_pcm;
    SpscQueue<ClockSpan, 512>& m_clock;
    std::uint64_t              m_output {};
    unsigned                   m_offset {};
    bool                       m_valid {};
};
} // namespace

class Player::Private {
public:
    using Receiver = rn::async::CompletionQueue<Command>;
    using Done     = rn::async::CompletionQueue<bool>;
    Private(std::string_view name, Notifier notifier, std::pmr::memory_resource* memory)
        : notify(std::move(notifier)),
          actions(rn::move(Receiver::make()).unwrap_unchecked()),
          done(rn::move(Done::make()).unwrap_unchecked()),
          pcm(memory),
          clock(memory),
          app_name(name) {}
    ~Private() {
        close();
        if (worker.joinable()) worker.join();
    }
    void submit(CommandKind kind, std::string source = {}, double value = 0) {
        std::lock_guard lock(ingress);
        if (closed) return;
        if (kind == CommandKind::Source || kind == CommandKind::Stop) ++requested_source;
        if (kind == CommandKind::Source) requested_playing = ! source.empty();
        if (kind == CommandKind::Play) requested_playing = true;
        if (kind == CommandKind::Pause || kind == CommandKind::Stop) requested_playing = false;
        if (kind == CommandKind::Source || kind == CommandKind::Stop || kind == CommandKind::Seek) {
            ++requested_revision;
            notify.advance_epoch();
        }
        (void)actions.get<1>().push(Command { kind,
                                              std::move(source),
                                              value,
                                              requested_revision.load(),
                                              requested_source.load(),
                                              ++latest_id });
    }
    void close() {
        std::lock_guard lock(ingress);
        if (closed.exchange(true)) return;
        ++requested_revision;
        notify.advance_epoch();
        actions.get<1>().close();
        cv.notify_all();
    }
    bool cancelled() const {
        return closed || active_revision != requested_revision.load() ||
               active_source != requested_source.load();
    }
    static bool interrupt(void* self) noexcept { return static_cast<Private*>(self)->cancelled(); }
    void        send(notify::info info) {
        std::lock_guard lock(ingress);
        if (! cancelled()) notify.send(std::move(info));
    }
    void busy(bool value) {
        if (cancelled() || (! value && current_id != latest_id) || last_busy == value) return;
        last_busy = value;
        send(notify::info::busy(value));
    }
    void state(PlayState value) {
        if (cancelled() || last_state == value) return;
        last_state = value;
        send(notify::info::playstate(value));
    }
    void position(i64 value) {
        if (cancelled() || last_position == value) return;
        last_position = value;
        send(notify::info::position(value));
    }
    void stop_device() {
        if (device) {
            device->shutdown();
            device->wait_stopped();
            device.reset();
        }
        pcm.reset();
        clock.reset();
        last_end     = 0;
        last_seconds = 0;
        fade_pending = false;
    }
    void stop_media() {
        stop_device();
        decoder.reset();
        desired       = {};
        playing       = false;
        start_pending = seek_pending = false;
        position(0);
        state(PlayState::Stopped);
    }
    bool open_device() {
        device             = std::make_unique<wa::AudioDevice>();
        desired            = {};
        desired.generation = rn::u64(++device_generation);
        desired.active     = true;
        desired.volume     = rn::f32(std::pow(volume.load(), 3.0f));
        desired.identity.application_name =
            rn::prelude::String::make(rn::cppstd::as_str(app_name).unwrap_or("Qcm"_str));
        desired.identity.application_id = rn::prelude::String::make("org.qcm.Qcm"_str);
        if (! device->mount(std::make_unique<PcmChannel>(pcm, clock), rn::u64(device_generation)) ||
            ! device->apply(desired.clone()))
            return false;
        const auto deadline = std::chrono::steady_clock::now() + 5s;
        while (device->state() != wa::AudioDeviceState::ReadyPaused) {
            if (cancelled() || device->state() == wa::AudioDeviceState::Failed ||
                std::chrono::steady_clock::now() >= deadline)
                return false;
            std::this_thread::sleep_for(2ms);
        }
        return true;
    }
    void fail() {
        stop_media();
        busy(false);
    }
    void set_source(const std::string& source) {
        stop_media();
        send(notify::info::duration(0));
        if (source.empty() || cancelled()) {
            busy(false);
            return;
        }
        auto url = rn::cppstd::as_str(source);
        if (url.is_err()) {
            fail();
            return;
        }
        auto media =
            wa::OpenedMedia::open(url.unwrap(), { .context = this, .cancelled = &interrupt });
        if (media.is_err()) {
            fail();
            return;
        }
        decoder = std::make_unique<wa::StreamDecoder>();
        if (! decoder->open(rn::move(media).unwrap(), { rn::u32(2), rn::u32(48000) })) {
            fail();
            return;
        }
        const auto& info = decoder->info();
        send(notify::info::duration(
            info.duration_seconds.is_some()
                ? static_cast<i64>((*info.duration_seconds).to_primitive() * 1000)
                : 0));
        if (! open_device()) {
            fail();
            return;
        }
        playing       = requested_playing.load();
        start_pending = true;
    }
    void set_playing(bool value) {
        playing = value;
        if (! device) {
            busy(false);
            return;
        }
        if (value) {
            fade_pending         = false;
            desired.playing      = true;
            desired.volume_scale = rn::f32(1.0);
        } else {
            if (! desired.playing) {
                state(PlayState::Paused);
                busy(false);
                return;
            }
            desired.volume_scale = rn::f32();
            fade_pending         = true;
        }
        desired.volume_scale_fade_ms = rn::u32(fade_time.load() / 1000);
        ++desired.volume_scale_revision;
        device->apply(desired.clone());
    }
    void seek(double milliseconds) {
        if (! decoder && ! current_source.empty()) set_source(current_source);
        if (! decoder || cancelled() || ! decoder->info().seekable) {
            busy(false);
            return;
        }
        const bool resume = requested_playing.load();
        stop_device();
        if (! decoder->seek_to(rn::f64(std::max(0.0, milliseconds / 1000)))) {
            if (! cancelled()) fail();
            return;
        }
        if (! open_device()) {
            if (cancelled())
                stop_device();
            else
                fail();
            return;
        }
        playing       = resume;
        start_pending = true;
        seek_pending  = true;
    }
    void command(Command command) {
        if (command.source_revision != requested_source) return;
        if (command.kind != CommandKind::Source && command.revision != requested_revision) return;
        const auto revision =
            command.kind == CommandKind::Source ? requested_revision.load() : command.revision;
        active_source = command.source_revision;
        if (active_revision != revision) {
            last_busy.reset();
            last_state.reset();
            last_position = -1;
        }
        active_revision = revision;
        current_id      = command.id;
        busy(true);
        switch (command.kind) {
        case CommandKind::Source:
            current_source = std::move(command.source);
            set_source(current_source);
            break;
        case CommandKind::Play: set_playing(true); break;
        case CommandKind::Pause: set_playing(false); break;
        case CommandKind::Stop:
            current_source.clear();
            stop_media();
            busy(false);
            break;
        case CommandKind::Seek: seek(command.value); break;
        case CommandKind::Volume:
            if (device) {
                desired.volume = rn::f32(std::pow(volume.load(), 3.0f));
                device->apply(desired.clone());
            }
            busy(false);
            break;
        }
    }
    void tick() {
        if (cancelled() || ! decoder || ! device) return;
        if (device->state() == wa::AudioDeviceState::Failed) {
            fail();
            return;
        }
        // Drain clock reports before decoding, including while paused.
        const auto played = device->stream_position_frames().to_primitive();
        while (const auto* span = clock.front()) {
            last_end     = span->output_frame + span->frames;
            last_seconds = span->seconds + double(span->frames) / 48000;
            if (played < span->output_frame) break;
            const auto offset = std::min<std::uint64_t>(played - span->output_frame, span->frames);
            position(static_cast<i64>((span->seconds + double(offset) / 48000) * 1000));
            if (offset < span->frames) break;
            clock.pop();
        }
        if (fade_pending &&
            device->completed_volume_scale_revision() == desired.volume_scale_revision) {
            fade_pending    = false;
            desired.playing = false;
            device->apply(desired.clone());
        }
        if (! start_pending && ! fade_pending) {
            if (playing && device->state() == wa::AudioDeviceState::ReadyPlaying) {
                state(PlayState::Playing);
                busy(false);
            }
            if (! playing && device->state() == wa::AudioDeviceState::ReadyPaused) {
                state(PlayState::Paused);
                busy(false);
            }
        }
        if (! pcm.full() && ! decoder->is_eof()) {
            PcmBlock block;
            block.frames = static_cast<unsigned>(
                decoder->next_pcm(block.samples.data(), rn::u32(1024)).to_primitive());
            block.seconds = decoder->pcm_position_seconds().to_primitive();
            if (cancelled()) return;
            if (decoder->error().kind != wa::MediaErrorKind::None) {
                fail();
                return;
            }
            if (block.frames) {
                pcm.push(block);
                if (seek_pending) {
                    position(static_cast<i64>(block.seconds * 1000));
                    seek_pending = false;
                }
            }
        }
        if (start_pending && (! pcm.empty() || decoder->is_eof())) {
            start_pending                = false;
            desired.volume_scale         = rn::f32();
            desired.volume_scale_fade_ms = rn::u32();
            ++desired.volume_scale_revision;
            device->apply(desired.clone());
            set_playing(playing);
        }
        if (decoder->is_eof() && pcm.empty() && clock.empty() && played >= last_end && playing) {
            position(static_cast<i64>(last_seconds * 1000));
            desired.playing = false;
            device->apply(desired.clone());
            playing = false;
            state(PlayState::Stopped);
            busy(false);
            send(notify::info::ended());
            stop_device();
            decoder.reset();
        }
    }
    void run() {
        while (! closed) {
            std::deque<Command> batch;
            {
                std::unique_lock lock(mutex);
                if (pending.empty()) {
                    if (device)
                        cv.wait_for(lock, 2ms);
                    else
                        cv.wait(lock, [this] {
                            return closed || ! pending.empty();
                        });
                }
                batch.swap(pending);
            }
            for (auto& item : batch) {
                if (closed) break;
                command(std::move(item));
            }
            tick();
        }
        stop_media();
        done.get<1>().close();
    }
    Notifier                                                       notify;
    rn::tuple<Receiver, rn::async::CompletionQueueHandle<Command>> actions;
    rn::tuple<Done, rn::async::CompletionQueueHandle<bool>>        done;
    std::mutex                                                     ingress, mutex;
    std::condition_variable                                        cv;
    std::deque<Command>                                            pending;
    std::thread                                                    worker;
    std::atomic<bool>                                              closed {};
    std::atomic<bool>                                              requested_playing {};
    std::atomic<std::uint64_t> requested_revision {}, requested_source {}, latest_id {};
    std::uint64_t      active_revision {}, active_source {}, current_id {}, device_generation {};
    std::atomic<float> volume { 1.0f };
    std::atomic<u32>   fade_time { 500000 };
    SpscQueue<PcmBlock, 32>            pcm;
    SpscQueue<ClockSpan, 512>          clock;
    std::string                        app_name, current_source;
    std::unique_ptr<wa::StreamDecoder> decoder;
    std::unique_ptr<wa::AudioDevice>   device;
    wa::AudioDeviceDesiredState        desired;
    bool                     playing {}, fade_pending {}, start_pending {}, seek_pending {};
    std::optional<bool>      last_busy;
    std::optional<PlayState> last_state;
    i64                      last_position { -1 };
    std::uint64_t            last_end {};
    double                   last_seconds {};
};

Player::Player(std::string_view name, Notifier notify, std::pmr::memory_resource* memory)
    : m_d(make_up<Private>(name, std::move(notify), memory)) {}
Player::~Player() = default;
auto Player::process_actions() -> rn::async::coro<void> {
    C_D(Player);
    d->worker = std::thread([d] {
        d->run();
    });
    for (;;) {
        auto result = co_await d->actions.get<0>().next();
        if (result.is_err()) break;
        auto item = rn::move(result).unwrap();
        if (item.is_none()) break;
        {
            std::lock_guard lock(d->mutex);
            d->pending.push_back(rn::move(item).unwrap());
        }
        d->cv.notify_one();
    }
    d->close();
    (void)co_await d->done.get<0>().next();
    if (d->worker.joinable()) d->worker.join();
}
void Player::close() { m_d->close(); }
void Player::set_source(std::string_view value) {
    m_d->submit(CommandKind::Source, std::string(value));
}
void Player::play() { m_d->submit(CommandKind::Play); }
void Player::pause() { m_d->submit(CommandKind::Pause); }
void Player::stop() { m_d->submit(CommandKind::Stop); }
void Player::seek(i32 value) { m_d->submit(CommandKind::Seek, {}, value); }
auto Player::volume() const -> float { return m_d->volume; }
void Player::set_volume(float value) {
    if (! std::isfinite(value)) return;
    m_d->volume = std::clamp(value, 0.0f, 1.0f);
    m_d->submit(CommandKind::Volume);
}
auto Player::fade_time() const -> u32 { return m_d->fade_time; }
void Player::set_fade_time(u32 value) { m_d->fade_time = value; }
