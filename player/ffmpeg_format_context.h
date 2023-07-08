#pragma once

extern "C" {
#include <libavformat/avformat.h>
}
#include <atomic>

#include "core/core.h"
#include "ffmpeg_error.h"

namespace player
{

class FFmpegFormatContext : NoCopy {
public:
    using Self = FFmpegFormatContext;
    FFmpegFormatContext(): m_d(avformat_alloc_context()), m_aborted(false) {
        m_d->interrupt_callback.opaque   = this;
        m_d->interrupt_callback.callback = decode_interrupt_cb;
    }
    ~FFmpegFormatContext() {
        avformat_close_input(&m_d);
        avformat_free_context(m_d);
    }

    FFmpegFormatContext(Self&& o): FFmpegFormatContext() { *this = std::move(o); }
    Self& operator=(Self&& o) {
        std::swap(m_d, o.m_d);
        m_aborted.store(o.m_aborted);
        return *this;
    }
    constexpr auto operator->() { return m_d; }
    constexpr auto operator->() const { return m_d; }

    FFmpegError open_input(const char* url) noexcept {
        return avformat_open_input(&m_d, url, NULL, NULL);
    }

    FFmpegError find_stream_info(AVDictionary** options) noexcept {
        return avformat_find_stream_info(m_d, options);
    }

    void dump_format(int idx, const char* url, bool is_output) {
        av_dump_format(m_d, idx, url, is_output);
    }

    FFmpegError read_frame(AVPacket* pkt) { return av_read_frame(m_d, pkt); }

    int find_best_stream(enum AVMediaType type, int wanted_stream_nb, int related_stream,
                         const AVCodec** decoder_ret, int flags) {
        return av_find_best_stream(m_d, type, wanted_stream_nb, related_stream, decoder_ret, flags);
    }

    FFmpegError seek_file(int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags) {
        return avformat_seek_file(m_d, stream_index, min_ts, ts, max_ts, flags);
    }

    void set_aborted(bool v) { m_aborted = v; }

    static int decode_interrupt_cb(void* self_) {
        auto self = static_cast<Self*>(self_);
        return self->m_aborted;
    }

private:
    AVFormatContext*  m_d;
    std::atomic<bool> m_aborted;
};

} // namespace player
