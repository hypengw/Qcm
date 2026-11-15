export module qcm.player:player;
import rstd;
import wavsen.audio;
import :pcm;
import :notify;

using namespace rstd::prelude;
using rstd::int64_t;
using rstd::tuple;
using rstd::uint32_t;
using rstd::uint64_t;
using rstd::alloc::Allocator;
using rstd::async::CompletionQueue;
using rstd::async::CompletionQueueHandle;
using rstd::async::coro;
using rstd::sync::Condvar;
using rstd::sync::Mutex;
using rstd::sync::atomic::Atomic;
using rstd::thread::JoinHandle;
namespace wa = wavsen::audio;

export namespace player
{

class Player {
public:
    Player(const Player&)                    = delete;
    auto operator=(const Player&) -> Player& = delete;
    Player(Player&&)                         = delete;
    auto operator=(Player&&) -> Player&      = delete;
    Player(ref<str>            name, Notifier,
           ref<dyn<Allocator>> allocator = ::alloc::allocator_ref(::alloc::GLOBAL));
    ~Player();

    auto process_actions() -> coro<void>;
    void close();

    void play(uint64_t token = 0);
    void pause(uint64_t token = 0);
    void stop(uint64_t token = 0);
    void seek(int64_t, uint64_t token = 0);

    void set_source(ref<str>, Option<Box<dyn<wa::MediaIo>>> io = {}, uint64_t token = 0,
                    bool autoplay = true);

    auto volume() const -> float;
    void set_volume(float, uint64_t token = 0);

    auto fade_time() const -> uint32_t;
    void set_fade_time(uint32_t);
    void set_fade_time_ms(uint32_t);
    auto pending_actions() const -> uint64_t { return queued_actions.load(); }

private:
    using QueueAllocator = ref<dyn<Allocator>>;
    template<class T>
    using Producer = rstd::sync::spsc::Producer<T, QueueAllocator>;
    template<class T>
    using Consumer = rstd::sync::spsc::Consumer<T, QueueAllocator>;
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
        CommandKind                   kind;
        String                        source;
        Option<Box<dyn<wa::MediaIo>>> media_io;
        double                        value {};
        uint64_t                      revision {}, source_revision {}, id {}, token {};
    };
    using Receiver = CompletionQueue<Command>;
    using Done     = CompletionQueue<bool>;
    void submit(CommandKind kind, String source = {}, double value = 0,
                Option<Box<dyn<wa::MediaIo>>> io = {}, uint64_t token = 0, bool autoplay = true);
    bool cancelled() const;
    static bool                                     interrupt(void* self) noexcept;
    void                                            send(notify::info info);
    void                                            busy(bool value);
    void                                            state(PlayState value);
    void                                            position(int64_t value);
    void                                            stop_device();
    void                                            stop_media();
    bool                                            open_device();
    void                                            fail(ref<str> message, uint64_t token = 0);
    void                                            report_seeked(int64_t position);
    void                                            open_source(ref<str> source);
    void                                            set_playing(bool value);
    void                                            seek_media(double milliseconds);
    void                                            command(Command command);
    void                                            tick();
    void                                            run();
    Notifier                                        notify;
    tuple<Receiver, CompletionQueueHandle<Command>> actions;
    tuple<Done, CompletionQueueHandle<bool>>        done;
    Mutex<empty>                                    ingress;
    Condvar                                         cv;
    Mutex<Vec<Command>>                             pending;
    Option<JoinHandle<void>>                        worker;
    Atomic<bool>                                    closed {};
    Atomic<bool>                                    requested_playing {};
    Atomic<uint64_t>    requested_revision {}, requested_source {}, latest_id {};
    Atomic<uint64_t>    queued_actions {};
    uint64_t            active_revision {}, active_source {}, current_id {}, device_generation {};
    uint64_t            current_token {}, source_token {}, seek_token {};
    Atomic<rstd::f32>   m_volume { rstd::f32(1) };
    Atomic<uint64_t>    m_fade_time { 500000 };
    QueueAllocator      queue_allocator;
    Producer<PcmBlock>  pcm;
    Consumer<ClockSpan> clock;
    String              app_name;
    String              current_source;
    Option<Box<dyn<wa::MediaIo>>>  media_io;
    Option<Box<wa::StreamDecoder>> decoder;
    Option<Box<wa::AudioDevice>>   device;
    wa::AudioDeviceDesiredState    desired;
    bool                           playing {}, fade_pending {}, start_pending {}, seek_pending {};
    Option<bool>                   last_busy;
    Option<PlayState>              last_state;
    int64_t                        last_position { -1 };
    uint64_t                       last_end {};
    double                         last_seconds {};
};

} // namespace player
