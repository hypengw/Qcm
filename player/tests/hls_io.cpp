#include <atomic>
#include <cstdio>
#include <string>
import rstd;
import rstd.cppstd;
import wavsen.audio;
using namespace rstd::prelude;
namespace wa = wavsen::audio;
class Bridge final : public wa::MediaIo {
public:
    std::string base;
    int         opens {}, closes {}, live {};
    auto        resolve(const wa::MediaIoRequest& request)
        -> Result<wa::MediaInput, wa::MediaError> override {
        auto                       uri = rstd::cppstd::to_string(request.uri);
        std::string                url;
        constexpr std::string_view logical = "http://qcm.invalid/";
        if (uri.starts_with(logical))
            url = base + uri.substr(logical.size());
        else if (uri.starts_with(base))
            url = uri;
        else
            return Err(wa::MediaError { wa::MediaErrorKind::InvalidInput });
        ++opens;
        ++live;
        return Ok(
            wa::MediaInput { String::make(rstd::cppstd::as_str(url).unwrap()), u64(opens), true });
    }
    void release(u64) noexcept override {
        ++closes;
        --live;
    }
};
bool cancelled(void* value) noexcept { return static_cast<std::atomic<bool>*>(value)->load(); }
int  main(int argc, char** argv) {
    if (argc < 2) return 0;
    Bridge bridge;
    bridge.base = argv[1];
    std::atomic<bool> stop { false };
    {
        auto opened = wa::OpenedMedia::open(
            rstd::cppstd::as_str("http://qcm.invalid/master.m3u8").unwrap(),
            { .context = &stop, .cancelled = &cancelled, .reconnect = false, .media_io = &bridge });
        if (opened.is_err()) return 1;
        wa::StreamDecoder decoder;
        if (! decoder.open(rstd::move(opened).unwrap(), { u32(2), u32(48000) })) return 2;
        if (! decoder.info().seekable) return 3;
        rstd::array<float, 2048> pcm {};
        for (double position : { 0.0, 12.0, 6.0 }) {
            if (! decoder.seek_to(f64(position))) return 4;
            u64 frames {};
            for (int n = 0; n < 8; ++n) frames += decoder.next_pcm(pcm.data(), u32(1024));
            if (frames == u64() || decoder.error().kind != wa::MediaErrorKind::None) return 5;
            const double offset = decoder.pcm_position_seconds().to_primitive() - position;
            if (offset < -0.05 || offset > 0.5) return 8;
            std::printf("seek=%.1f pcm_position=%.3f\n",
                        position,
                        decoder.pcm_position_seconds().to_primitive());
        }
        stop = true;
        (void)decoder.next_pcm(pcm.data(), u32(1024));
        if (decoder.error().kind != wa::MediaErrorKind::Cancelled) return 6;
    }
    std::printf("opens=%d closes=%d live=%d\n", bridge.opens, bridge.closes, bridge.live);
    return bridge.live == 0 && bridge.opens == bridge.closes ? 0 : 7;
}
