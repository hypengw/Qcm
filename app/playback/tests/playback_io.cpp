#include <QCoreApplication>
#include <QUrl>
#include <atomic>
#include <cstdio>
#include <chrono>
#include <filesystem>
#ifdef __linux__
#    include <sys/resource.h>
#endif
#include <thread>
import qcm.playback.io;
import rstd;
import rstd.cppstd;
import wavsen.audio;
using namespace rstd::prelude;
namespace wa = wavsen::audio;
bool cancelled(void* value) noexcept { return static_cast<std::atomic<bool>*>(value)->load(); }
int  run(int argc, char** argv) {
    if (argc < 2) return 0;
    QUrl source(QString::fromUtf8(argv[1]));
    auto io = qcm::make_playback_io(source);
    if (! io) return 1;
    std::atomic<bool> stop { false };
    size_t            descriptors = 0;
    for (int attempt = 0; attempt < 2; ++attempt) {
        const auto opened_at = std::chrono::steady_clock::now();
        {
            auto opened = wa::OpenedMedia::open(
                rstd::cppstd::as_str(source.toString().toStdString()).unwrap(),
                { .context   = &stop,
                  .cancelled = &cancelled,
                  .reconnect = false,
                  .media_io  = io.get() });
            if (opened.is_err()) return 2;
            wa::StreamDecoder decoder;
            if (! decoder.open(rstd::move(opened).unwrap(), { u32(2), u32(48000) })) return 3;
            if (! decoder.info().seekable) return 4;
            std::printf("open=%d ready_ms=%lld\n",
                        attempt,
                        (long long)std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - opened_at)
                            .count());
            rstd::array<float, 2048> pcm {};
            const double             long_positions[]  = { 2200.0, 4399.0, 0.0 };
            const double             short_positions[] = { 0.0, 12.0, 6.0 };
            const auto*              positions = argc > 2 ? short_positions : long_positions;
            for (int i = 0; i < 3; ++i) {
                double     position   = positions[i];
                const auto seeking_at = std::chrono::steady_clock::now();
                if (! decoder.seek_to(f64(position))) return 5;
                u64 frames {};
                for (int n = 0; n < 8; ++n) frames += decoder.next_pcm(pcm.data(), u32(1024));
                if (frames == u64() || decoder.error().kind != wa::MediaErrorKind::None) return 6;
                const double offset = decoder.pcm_position_seconds().to_primitive() - position;
                if (offset < -0.05 || offset > 0.5) return 8;
                std::printf("open=%d seek=%.1f pcm_position=%.3f seek_ms=%lld\n",
                            attempt,
                            position,
                            decoder.pcm_position_seconds().to_primitive(),
                            (long long)std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - seeking_at)
                                .count());
            }
            io->keep_alive();
            stop = true;
            (void)decoder.next_pcm(pcm.data(), u32(1024));
            if (decoder.error().kind != wa::MediaErrorKind::Cancelled) return 7;
        }
        stop = false;
        io->close();
#ifdef __linux__
        const size_t  count = std::distance(std::filesystem::directory_iterator("/proc/self/fd"),
                                            std::filesystem::directory_iterator());
        struct rusage usage {};
        getrusage(RUSAGE_SELF, &usage);
        std::printf("open=%d fds=%zu peak_kib=%ld\n", attempt, count, usage.ru_maxrss);
        if (attempt && count > descriptors + 1) return 9;
        descriptors = count;
#endif
    }
    return 0;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) return 0;
    int         result = 0;
    std::thread worker([&] {
        result = run(argc, argv);
        QMetaObject::invokeMethod(&app, &QCoreApplication::quit, Qt::QueuedConnection);
    });
    app.exec();
    worker.join();
    return result;
}
