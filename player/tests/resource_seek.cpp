#include <atomic>
#include <cstdio>
import rstd;
import wavsen.audio;
using namespace rstd::prelude;
namespace wa = wavsen::audio;

bool cancelled(void* context) noexcept {
    return static_cast<std::atomic<bool>*>(context)->load();
}
int main(int argc, char** argv) {
    if (argc < 2) return 0;
    auto url = rstd::ffi::CStr::from_ptr(argv[1]).to_str().unwrap();
    std::atomic<bool> stop { true };
    auto cancelled_open = wa::OpenedMedia::open(url, { .context = &stop, .cancelled = &cancelled });
    if (cancelled_open.is_ok() || cancelled_open.unwrap_err().kind != wa::MediaErrorKind::Cancelled)
        return 1;
    stop = false;
    for (int attempt = 0; attempt < 2; ++attempt) {
        auto opened = wa::OpenedMedia::open(
            url, { .context = &stop, .cancelled = &cancelled, .reconnect = false });
        if (opened.is_err()) return 2;
        wa::StreamDecoder decoder;
        if (! decoder.open(rstd::move(opened).unwrap(), { u32(2), u32(48000) })) return 3;
        if (! decoder.info().seekable) return 4;
        rstd::array<float, 2048> samples {};
        const double positions[] = { 2200.0, 4399.0, 0.0 };
        for (double position : positions) {
            if (! decoder.seek_to(f64(position))) return 5;
            u64 frames {};
            for (int count = 0; count < 8; ++count)
                frames += decoder.next_pcm(samples.data(), u32(1024));
            if (frames == u64() || decoder.error().kind != wa::MediaErrorKind::None) return 6;
            std::printf("open=%d seek=%.1f decoded PCM\n", attempt, position);
        }
        stop = true;
        (void)decoder.next_pcm(samples.data(), u32(1024));
        if (decoder.error().kind != wa::MediaErrorKind::Cancelled) return 7;
        stop = false;
    }
    return 0;
}
