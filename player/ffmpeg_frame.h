#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

#include <utility>

#include "core/core.h"
#include "ffmpeg_error.h"

namespace player
{

class FFmpegFrame : NoCopy {
public:
    using Self = FFmpegFrame;

    FFmpegFrame(): m_frame(av_frame_alloc()) {}
    ~FFmpegFrame() { av_frame_free(&m_frame); }

    FFmpegFrame(Self&& o): FFmpegFrame() { *this = std::move(o); }
    Self& operator=(Self&& o) noexcept {
        std::swap(m_frame, o.m_frame);
        return *this;
    }

    static FFmpegResult<Self> ref(const AVFrame* in) {
        Self        out;
        FFmpegError err = av_frame_ref(out.m_frame, in);
        if (err) return rstd::Err(err);
        return rstd::Ok(std::move(out));
    }

    auto operator->() { return m_frame; }
    auto operator->() const { return m_frame; }

    void set_ch_layout(const AVChannelLayout& in) {
        av_channel_layout_copy(&(m_frame->ch_layout), &in);
    }

    void unref() { av_frame_unref(m_frame); }

    Self move_ref() {
        Self out;
        av_frame_move_ref(out.m_frame, m_frame);
        return out;
    }

    auto ref() const { return Self::ref(m_frame); }
    auto raw() const { return m_frame; }
    auto praw() { return &m_frame; }

    FFmpegError copy_props(const Self& o) {
        return FFmpegError(av_frame_copy_props(m_frame, o.m_frame)).record();
    }

private:
    AVFrame* m_frame;
};

} // namespace player
