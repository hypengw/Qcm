#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

#include "core/core.h"

namespace player
{

struct AudioParams {
    AudioParams() = default;
    AudioParams(const AudioParams& o) { *this = o; }
    AudioParams& operator=(const AudioParams& o) {
        format      = o.format;
        sample_rate = o.sample_rate;
        av_channel_layout_copy(&ch_layout, &o.ch_layout);
        return *this;
    }

    void set_ch_layout(const AVChannelLayout& in) { av_channel_layout_copy(&ch_layout, &in); }
    auto bytes_per_sample() const { return av_get_bytes_per_sample(format); }

    AVSampleFormat  format { AV_SAMPLE_FMT_NONE };
    i32             sample_rate { 1000 };
    AVChannelLayout ch_layout {};
};
} // namespace player
