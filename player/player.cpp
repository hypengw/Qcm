module qcm.player;
import :player;
import :pcm;
import qcm.core;

import rstd;
import rstd.cppstd;
import wavsen.audio;

using namespace player;
using namespace rstd::prelude;
using rstd::alloc::Allocator;
using rstd::async::coro;
using rstd::time::Duration;
using rstd::time::Instant;
namespace wa = wavsen::audio;
namespace rn = rstd;
using namespace rstd::literals;
namespace spsc       = rn::sync::spsc;
using QueueAllocator = ref<dyn<Allocator>>;
template<class T>
using Producer = spsc::Producer<T, QueueAllocator>;
template<class T>
using Consumer = spsc::Consumer<T, QueueAllocator>;

namespace
{
class PcmChannel : public wa::IPullChannel {
public:
    PcmChannel(Consumer<PcmBlock> pcm, Producer<ClockSpan> clock)
        : m_pcm(rn::move(pcm)), m_clock(rn::move(clock)) {}
    void pass_desc(const wa::DeviceDesc& desc) override {
        m_valid = desc.channels == rn::u32(2) && desc.sample_rate == rn::u32(48000);
    }
    void output_offset(rn::u64 value) override { m_output = value; }
    auto next_pcm(void* data, rn::u32 requested) -> rn::u64 override {
        if (! m_valid) return rn::u64();
        rn::u32 produced {};
        while (produced < requested) {
            auto front = m_pcm.try_front();
            if (front.is_err()) break;
            auto       block = rn::move(front).unwrap();
            const auto count = rn::cmp::min(block->frames - m_offset, requested - produced);
            if (m_clock
                    .try_push(ClockSpan { m_output + rn::u64(produced.to_primitive()),
                                          count,
                                          block->seconds +
                                              rn::f64(m_offset.to_primitive()) / rn::f64(48000) })
                    .is_err())
                break;
            rn::ptr_::copy_nonoverlapping(
                ptr<float>::from_raw_parts(block->samples.data() + m_offset.to_primitive() * 2),
                mut_ptr<float>::from_raw_parts(static_cast<float*>(data) +
                                               produced.to_primitive() * 2),
                rn::usize(count.to_primitive()) * rn::usize(2));
            produced += count;
            m_offset += count;
            if (m_offset == block->frames) {
                m_offset = rn::u32();
                block.consume();
            }
        }
        return rn::u64(produced.to_primitive());
    }

private:
    Consumer<PcmBlock>  m_pcm;
    Producer<ClockSpan> m_clock;
    rn::u64             m_output {};
    rn::u32             m_offset {};
    bool                m_valid {};
};
} // namespace

Player::Player(std::string_view name, Notifier notifier, QueueAllocator allocator)
    : notify(rn::move(notifier)),
      actions(rn::move(Receiver::make()).unwrap_unchecked()),
      done(rn::move(Done::make()).unwrap_unchecked()),
      queue_allocator(allocator),
      app_name(String::make(rn::cppstd::as_str(name).unwrap_or("Qcm"_str))) {}

Player::~Player() {
    close();
    if (worker.is_some()) (void)rn::move(worker.take()).unwrap().join();
}

void Player::submit(CommandKind kind, std::string source, double value) {
    auto lock = ingress.lock().unwrap();
    if (closed.load()) return;
    if (kind == CommandKind::Source || kind == CommandKind::Stop)
        (void)requested_source.fetch_add(1);
    if (kind == CommandKind::Source) requested_playing.store(! source.empty());
    if (kind == CommandKind::Play) requested_playing.store(true);
    if (kind == CommandKind::Pause || kind == CommandKind::Stop) requested_playing.store(false);
    if (kind == CommandKind::Source || kind == CommandKind::Stop || kind == CommandKind::Seek) {
        (void)requested_revision.fetch_add(1);
        notify.advance_epoch();
    }
    (void)actions.get<1>().push(Command { kind,
                                          rn::move(source),
                                          value,
                                          requested_revision.load(),
                                          requested_source.load(),
                                          (latest_id.fetch_add(1) + 1) });
}

void Player::close() {
    auto lock          = ingress.lock().unwrap();
    auto pending_guard = pending.lock().unwrap();
    if (closed.exchange(true)) return;
    (void)requested_revision.fetch_add(1);
    notify.advance_epoch();
    actions.get<1>().close();
    cv.notify_all();
}

bool Player::cancelled() const {
    return closed.load() || active_revision != requested_revision.load() ||
           active_source != requested_source.load();
}

bool Player::interrupt(void* self) noexcept { return static_cast<Player*>(self)->cancelled(); }

void Player::send(notify::info info) {
    auto lock = ingress.lock().unwrap();
    if (! cancelled()) notify.send(rn::move(info));
}

void Player::busy(bool value) {
    if (cancelled() || (! value && current_id != latest_id.load()) ||
        (last_busy.is_some() && *last_busy == value))
        return;
    last_busy = Some(value);
    send(notify::info::busy(value));
}

void Player::state(PlayState value) {
    if (cancelled() || (last_state.is_some() && *last_state == value)) return;
    last_state = Some(value);
    send(notify::info::playstate(value));
}

void Player::position(int64_t value) {
    if (cancelled() || last_position == value) return;
    last_position = value;
    send(notify::info::position(value));
}

void Player::stop_device() {
    if (device) {
        (*device)->shutdown();
        (*device)->wait_stopped();
        device = None();
    }
    pcm.close();
    clock.close();
    last_end     = 0;
    last_seconds = 0;
    fade_pending = false;
}

void Player::stop_media() {
    stop_device();
    decoder       = None();
    desired       = {};
    playing       = false;
    start_pending = seek_pending = false;
    position(0);
    state(PlayState::Stopped);
}

bool Player::open_device() {
    auto pcm_queue =
        spsc::RingBuffer<PcmBlock, QueueAllocator>::make(rn::usize(32), queue_allocator);
    auto clock_queue =
        spsc::RingBuffer<ClockSpan, QueueAllocator>::make(rn::usize(512), queue_allocator);
    if (pcm_queue.is_err() || clock_queue.is_err()) return false;
    auto pcm_pair                     = rn::move(pcm_queue).unwrap();
    auto clock_pair                   = rn::move(clock_queue).unwrap();
    pcm                               = rn::move(pcm_pair.get<0>());
    clock                             = rn::move(clock_pair.get<1>());
    device                            = Some(Box<wa::AudioDevice>::make());
    desired                           = {};
    desired.generation                = rn::u64(++device_generation);
    desired.active                    = true;
    desired.volume                    = m_volume.load().powf(rn::f32(3));
    desired.identity.application_name = app_name.clone();
    desired.identity.application_id   = String::make("org.qcm.Qcm"_str);
    if (! (*device)->mount(std::make_unique<PcmChannel>(rn::move(pcm_pair.get<1>()),
                                                        rn::move(clock_pair.get<0>())),
                           rn::u64(device_generation)) ||
        ! (*device)->apply(desired.clone()))
        return false;
    const auto started = Instant::now();
    while ((*device)->state() != wa::AudioDeviceState::ReadyPaused) {
        if (cancelled() || (*device)->state() == wa::AudioDeviceState::Failed ||
            started.elapsed() >= Duration::from_secs(rn::u64(5)))
            return false;
        rn::thread::sleep(Duration::from_millis(rn::u64(2)));
    }
    return true;
}

void Player::fail() {
    stop_media();
    busy(false);
}

void Player::open_source(const std::string& source) {
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
    auto media = wa::OpenedMedia::open(url.unwrap(), { .context = this, .cancelled = &interrupt });
    if (media.is_err()) {
        fail();
        return;
    }
    decoder = Some(Box<wa::StreamDecoder>::make());
    if (! (*decoder)->open(rn::move(media).unwrap(), { rn::u32(2), rn::u32(48000) })) {
        fail();
        return;
    }
    const auto& info = (*decoder)->info();
    send(notify::info::duration(
        info.duration_seconds.is_some()
            ? static_cast<int64_t>((*info.duration_seconds).to_primitive() * 1000)
            : 0));
    if (! open_device()) {
        fail();
        return;
    }
    playing       = requested_playing.load();
    start_pending = true;
}

void Player::set_playing(bool value) {
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
    desired.volume_scale_fade_ms = rn::u32(m_fade_time.load() / 1000);
    ++desired.volume_scale_revision;
    (*device)->apply(desired.clone());
}

void Player::seek_media(double milliseconds) {
    if (! decoder && ! current_source.empty()) open_source(current_source);
    if (! decoder || cancelled() || ! (*decoder)->info().seekable) {
        busy(false);
        return;
    }
    const bool resume = requested_playing.load();
    stop_device();
    if (! (*decoder)->seek_to(rn::f64(milliseconds / 1000).max(rn::f64()))) {
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

void Player::command(Command command) {
    if (command.source_revision != requested_source.load()) return;
    if (command.kind != CommandKind::Source && command.revision != requested_revision.load())
        return;
    const auto revision =
        command.kind == CommandKind::Source ? requested_revision.load() : command.revision;
    active_source = command.source_revision;
    if (active_revision != revision) {
        last_busy     = None();
        last_state    = None();
        last_position = -1;
    }
    active_revision = revision;
    current_id      = command.id;
    busy(true);
    switch (command.kind) {
    case CommandKind::Source:
        current_source = rn::move(command.source);
        open_source(current_source);
        break;
    case CommandKind::Play: set_playing(true); break;
    case CommandKind::Pause: set_playing(false); break;
    case CommandKind::Stop:
        current_source.clear();
        stop_media();
        busy(false);
        break;
    case CommandKind::Seek: seek_media(command.value); break;
    case CommandKind::Volume:
        if (device) {
            desired.volume = m_volume.load().powf(rn::f32(3));
            (*device)->apply(desired.clone());
        }
        busy(false);
        break;
    }
}

void Player::tick() {
    if (cancelled() || ! decoder || ! device) return;
    if ((*device)->state() == wa::AudioDeviceState::Failed) {
        fail();
        return;
    }
    // Drain clock reports before decoding, including while paused.
    const auto played = (*device)->stream_position_frames().to_primitive();
    for (;;) {
        auto front = clock.try_front();
        if (front.is_err()) break;
        auto span    = rn::move(front).unwrap();
        last_end     = span->output_frame.to_primitive() + span->frames.to_primitive();
        last_seconds = span->seconds.to_primitive() + double(span->frames.to_primitive()) / 48000;
        if (played < span->output_frame.to_primitive()) break;
        const auto offset =
            rn::cmp::min(rn::u64(played) - span->output_frame, rn::u64(span->frames.to_primitive()))
                .to_primitive();
        position(
            static_cast<int64_t>((span->seconds.to_primitive() + double(offset) / 48000) * 1000));
        if (offset < span->frames.to_primitive()) break;
        span.consume();
    }
    if (fade_pending &&
        (*device)->completed_volume_scale_revision() == desired.volume_scale_revision) {
        fade_pending    = false;
        desired.playing = false;
        (*device)->apply(desired.clone());
    }
    if (! start_pending && ! fade_pending) {
        if (playing && (*device)->state() == wa::AudioDeviceState::ReadyPlaying) {
            state(PlayState::Playing);
            busy(false);
        }
        if (! playing && (*device)->state() == wa::AudioDeviceState::ReadyPaused) {
            state(PlayState::Paused);
            busy(false);
        }
    }
    if (! pcm.is_full() && ! (*decoder)->is_eof()) {
        PcmBlock block;
        block.frames =
            rn::u32((*decoder)->next_pcm(block.samples.data(), rn::u32(1024)).to_primitive());
        block.seconds = (*decoder)->pcm_position_seconds();
        if (cancelled()) return;
        if ((*decoder)->error().kind != wa::MediaErrorKind::None) {
            fail();
            return;
        }
        if (block.frames != rn::u32()) {
            const auto seconds = block.seconds.to_primitive();
            if (pcm.try_push(rn::move(block)).is_err()) {
                fail();
                return;
            }
            if (seek_pending) {
                position(static_cast<int64_t>(seconds * 1000));
                seek_pending = false;
            }
        }
    }
    if (start_pending && (! pcm.is_empty() || (*decoder)->is_eof())) {
        start_pending                = false;
        desired.volume_scale         = rn::f32();
        desired.volume_scale_fade_ms = rn::u32();
        ++desired.volume_scale_revision;
        (*device)->apply(desired.clone());
        set_playing(playing);
    }
    if ((*decoder)->is_eof() && pcm.is_empty() && clock.is_empty() && played >= last_end &&
        playing) {
        position(static_cast<int64_t>(last_seconds * 1000));
        desired.playing = false;
        (*device)->apply(desired.clone());
        playing = false;
        state(PlayState::Stopped);
        busy(false);
        send(notify::info::ended());
        stop_device();
        decoder = None();
    }
}

void Player::run() {
    while (! closed.load()) {
        Vec<Command> batch;
        {
            auto lock = pending.lock().unwrap();
            if (lock->is_empty()) {
                if (device)
                    (void)cv.wait_timeout(lock, Duration::from_millis(rn::u64(2)));
                else
                    cv.wait_while(lock, [this](const auto& items) {
                        return ! closed.load() && items.is_empty();
                    });
            }
            batch = rn::move(*lock);
        }
        for (auto& item : batch) {
            if (closed.load()) break;
            command(rn::move(item));
        }
        tick();
    }
    stop_media();
    done.get<1>().close();
}

auto Player::process_actions() -> coro<void> {
    worker = Some(rn::thread::spawn([this] {
                      run();
                  }).unwrap());
    for (;;) {
        auto result = co_await actions.get<0>().next();
        if (result.is_err()) break;
        auto item = rn::move(result).unwrap();
        if (item.is_none()) break;
        {
            auto lock = pending.lock().unwrap();
            lock->push(rn::move(item).unwrap());
        }
        cv.notify_one();
    }
    close();
    (void)co_await done.get<0>().next();
    if (worker.is_some()) (void)rn::move(worker.take()).unwrap().join();
}
void Player::set_source(std::string_view value) { submit(CommandKind::Source, std::string(value)); }
void Player::play() { submit(CommandKind::Play); }
void Player::pause() { submit(CommandKind::Pause); }
void Player::stop() { submit(CommandKind::Stop); }
void Player::seek(int32_t value) { submit(CommandKind::Seek, {}, value); }
auto Player::volume() const -> float { return m_volume.load().to_primitive(); }
void Player::set_volume(float value) {
    if (! rn::f32(value).is_finite()) return;
    m_volume.store(rn::f32(value).clamp(rn::f32(), rn::f32(1)));
    submit(CommandKind::Volume);
}
auto Player::fade_time() const -> uint32_t { return m_fade_time.load(); }
void Player::set_fade_time(uint32_t value) { m_fade_time.store(value); }
