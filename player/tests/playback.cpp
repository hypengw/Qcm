#include <rstd/enum.hpp>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
import qcm.player;
import qcm.core;
import rstd;
using namespace std::chrono_literals;
class Sink : public player::detail::NotifySink {
public:
    bool send(player::notify::info info) override {
        RSTD_MATCH(info) {
            RSTD_CASE(position, value) { position = value; }
            RSTD_CASE(playstate, value) { state = value; }
            RSTD_CASE(busy, value) { busy = value; }
            RSTD_CASE(ended) { ++ended; }
            RSTD_CASE(duration, value) {}
            RSTD_CASE(cache, begin, end) {}
        }
        return true;
    }
    auto                           advance_epoch() -> u64 override { return ++epoch; }
    std::atomic<u64>               epoch {};
    std::atomic<i64>               position {};
    std::atomic<player::PlayState> state { player::PlayState::Stopped };
    std::atomic<int>               ended {};
    std::atomic<bool>              busy {};
};
template<class F>
bool wait(F predicate, std::chrono::milliseconds timeout = 5s) {
    const auto end = std::chrono::steady_clock::now() + timeout;
    while (! predicate()) {
        if (std::chrono::steady_clock::now() >= end) return false;
        std::this_thread::sleep_for(5ms);
    }
    return true;
}
int main(int argc, char** argv) {
    auto                     sink = make_rc<Sink>();
    qcm::MemoryStatAllocator memory;
    player::Player player("Qcm test", player::Notifier(sink), ::alloc::allocator_ref(memory));
    std::thread    run([&] {
        rstd::async::block_on(player.process_actions());
    });
    auto           finish = [&](int result) {
        player.close();
        player.close();
        run.join();
        return memory.current_bytes() == 0 && memory.current_block_count() == 0 ? result : 30;
    };
    if (argc < 2) {
        player.set_source("/nonexistent/qcm-media-test");
        std::this_thread::sleep_for(50ms);
        if (! wait([&] {
                return ! sink->busy;
            }))
            return finish(2);
        return finish(sink->ended ? 3 : 0);
    }
    player.set_fade_time(0);
    player.set_source(argv[1]);
    if (argc > 3) {
        std::this_thread::sleep_for(100ms);
        const auto start = std::chrono::steady_clock::now();
        player.set_source(argv[3]);
        player.seek(200);
        player.pause();
        if (! wait([&] {
                return sink->state == player::PlayState::Paused && sink->position >= 190 &&
                       sink->position < 300 && ! sink->busy;
            }))
            return finish(21);
        return finish(std::chrono::steady_clock::now() - start < 1s ? 0 : 22);
    }
    if (argc > 2) {
        std::this_thread::sleep_for(100ms);
        const auto start  = std::chrono::steady_clock::now();
        const int  result = finish(0);
        return std::chrono::steady_clock::now() - start < 1s ? result : 20;
    }
    player.seek(200);
    if (! wait([&] {
            return sink->position > 100;
        }))
        return finish(4);
    player.pause();
    if (! wait([&] {
            return sink->state == player::PlayState::Paused && ! sink->busy;
        }))
        return finish(5);
    std::this_thread::sleep_for(100ms);
    const auto position = sink->position.load();
    std::this_thread::sleep_for(100ms);
    if (sink->position != position) return finish(6);
    player.seek(0);
    if (! wait([&] {
            return sink->position < 100 && ! sink->busy;
        }))
        return finish(7);
    player.set_fade_time(50000);
    player.play();
    if (! wait([&] {
            return sink->position > 100;
        }))
        return finish(8);
    player.pause();
    if (! wait([&] {
            return sink->state == player::PlayState::Paused && ! sink->busy;
        }))
        return finish(9);
    player.seek(500);
    std::this_thread::sleep_for(5ms);
    player.seek(200);
    if (! wait([&] {
            return sink->position >= 190 && sink->position < 300 && ! sink->busy;
        }))
        return finish(10);
    player.play();
    if (! wait(
            [&] {
                return sink->ended == 1;
            },
            8s))
        return finish(11);
    player.stop();
    std::this_thread::sleep_for(100ms);
    if (sink->ended != 1) return finish(12);
    player.set_source(argv[1]);
    player.set_source("/nonexistent/qcm-media-test");
    std::this_thread::sleep_for(100ms);
    if (sink->ended != 1) return finish(13);
    return finish(0);
}
