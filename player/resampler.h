#pragma once

#include "ffmpeg_frame.h"
#include <cstdint>
#include <libavutil/error.h>
#include <libavutil/rational.h>
#include <limits>
extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include "audio_stream_params.h"
#include "audio_frame.h"
#include "ffmpeg_error.h"

namespace player
{

class Resampler {
public:
    using Self = Resampler;
    Resampler(): m_ctx(swr_alloc()) {}
    ~Resampler() { swr_free(&m_ctx); }

    FFmpegError configure(AudioFrame& out, const AudioFrame& in) {
        FFmpegError err = swr_config_frame(m_ctx, out.ff.raw(), in.ff.raw());
        if (err) return err.record();
        err = swr_init(m_ctx);
        return err.record();
    }

    FFmpegError resampler(AudioFrame& out, const AudioFrame& in) {
        if (out.ff->pts != AV_NOPTS_VALUE) {
            double multiple_base = out.ff->sample_rate * in.ff->sample_rate;
            double  inpts         = in.ff->pts  * av_q2d(in.ff->time_base);
            double  outpts        = next_pts(inpts * multiple_base) / (double)multiple_base;
            out.ff->pts        = outpts / av_q2d(in.ff->time_base);
        } else {
            out.ff->pts = AV_NOPTS_VALUE;
        }
        return FFmpegError(swr_convert_frame(m_ctx, out.ff.raw(), in.ff.raw())).record();
    }

    FFmpegError remaining(AudioFrame& out) {
        return FFmpegError(swr_convert_frame(m_ctx, out.ff.raw(), NULL)).record();
    }

    bool is_inited() { return swr_is_initialized(m_ctx); }

    void close() { swr_close(m_ctx); }

    i64 get_delay() {
        return swr_get_delay(m_ctx, 1000);
    }

    i64 next_pts(i64 t = INT64_MIN) {
        if (! is_inited()) t = INT64_MIN;
        return swr_next_pts(m_ctx, t);
    }

private:
    SwrContext* m_ctx;
};

} // namespace player
