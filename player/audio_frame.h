#pragma once

#include <chrono>

#include "ffmpeg_frame.h"
#include "audio_stream_params.h"
#include "ffmpeg_error.h"

#include "core/core.h"

namespace player
{
using namespace std::chrono;

struct AudioFrame : NoCopy {
    using Self = AudioFrame;

    AudioFrame()  = default;
    ~AudioFrame() = default;
    AudioFrame(Self&& o) noexcept { *this = std::move(o); }
    Self& operator=(Self&& o) noexcept {
        ff = std::move(o.ff);
        return *this;
    }

    static Self               from(FFmpegFrame&& ff) { return Self(std::move(ff)); }
    static FFmpegResult<Self> from(const FFmpegFrame& ff) {
        auto ff_ref = ff.ref();
        return RECORD(ff_ref.map([](FFmpegFrame& ff) {
            return from(std::move(ff));
        }));
    }

    bool is_planar() const { return av_sample_fmt_is_planar((AVSampleFormat)ff->format); }

    AudioParams params() const {
        AudioParams out;
        out.format      = (AVSampleFormat)ff->format;
        out.sample_rate = ff->sample_rate;
        av_channel_layout_copy(&out.ch_layout, &(ff->ch_layout));
        return out;
    }

    void set_params(const AudioParams& in) {
        ff->format      = in.format;
        ff->sample_rate = in.sample_rate;
        av_channel_layout_copy(&(ff->ch_layout), &in.ch_layout);
    }

    FFmpegResult<int> buffer_size() const {
        int ret = av_samples_get_buffer_size(
            NULL, ff->ch_layout.nb_channels, ff->nb_samples, (AVSampleFormat)ff->format, 1);
        if (ret < 0) return UNEXPECTED(ret);
        return ret;
    }

    auto channel_data(i32 idx) const {
        _assert_msg_(idx < ff->ch_layout.nb_channels, "not such channel: {}", idx);
        usize buffer_size_ = buffer_size().value_or(0);
        return std::span<const byte>((const byte*)ff->extended_data[idx], buffer_size_);
    }

    void set_eof() { ff->pts = -2; }
    bool eof() const { return ff->pts == -2; }

    auto pts_duration() {
        return microseconds(
            av_rescale_q(ff->pts, ff->time_base, av_make_q(std::micro::num, std::micro::den)));
    }

    FFmpegFrame ff;

private:
    AudioFrame(FFmpegFrame&& ff_) noexcept: ff(std::move(ff_)) {}
};

} // namespace player
