export module qcm.player:player;
import qcm.core;
import rstd;
import rstd.cppstd;
import wavsen.audio;
import :pcm;
import :notify;

using namespace rstd::prelude;
using rstd::tuple;
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
    Player(std::string_view    name, Notifier,
           ref<dyn<Allocator>> allocator = ::alloc::allocator_ref(::alloc::GLOBAL));
    ~Player();

    auto process_actions() -> coro<void>;
    void close();

    void play();
    void pause();
    void stop();
    void seek(int32_t);

    void set_source(std::string_view);

    auto volume() const -> float;
    void set_volume(float);

    auto fade_time() const -> uint32_t;
    void set_fade_time(uint32_t);

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
        CommandKind kind;
        std::string source;
        double      value {};
        uint64_t    revision {}, source_revision {}, id {};
    };
    using Receiver = CompletionQueue<Command>;
    using Done     = CompletionQueue<bool>;
    void        submit(CommandKind kind, std::string source = {}, double value = 0);
    bool        cancelled() const;
    static bool interrupt(void* self) noexcept;
    void        send(notify::info info);
    void        busy(bool value);
    void        state(PlayState value);
    void        position(int64_t value);
    void        stop_device();
    void        stop_media();
    bool        open_device();
    void        fail();
    void        open_source(const std::string& source);
    void        set_playing(bool value);
    void        seek_media(double milliseconds);
    void        command(Command command);
    void        tick();
    void        run();
    Notifier    notify;
    tuple<Receiver, CompletionQueueHandle<Command>> actions;
    tuple<Done, CompletionQueueHandle<bool>>        done;
    Mutex<empty>                                    ingress;
    Condvar                                         cv;
    Mutex<Vec<Command>>                             pending;
    Option<JoinHandle<void>>                        worker;
    Atomic<bool>                                    closed {};
    Atomic<bool>                                    requested_playing {};
    Atomic<uint64_t>    requested_revision {}, requested_source {}, latest_id {};
    uint64_t            active_revision {}, active_source {}, current_id {}, device_generation {};
    Atomic<rstd::f32>   m_volume { rstd::f32(1) };
    Atomic<uint32_t>    m_fade_time { 500000 };
    QueueAllocator      queue_allocator;
    Producer<PcmBlock>  pcm;
    Consumer<ClockSpan> clock;
    String              app_name;
    std::string         current_source;
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
