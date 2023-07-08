#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>

#include "ffmpeg_format_context.h"
#include "core/log.h"
#include "ffmpeg_error.h"
#include "packet_queue.h"
#include "player/notify.h"

namespace player
{

class StreamReader : NoCopy {
public:
    struct StreamInfo {
        rc<FFmpegFormatContext> ctx;
        AVStream**              st;
        int                     audio_idx;
    };

    StreamReader(Notifier notifier)
        : m_st_idx({ -1 }),
          m_promise_stream_info(),
          m_future_stream_info(m_promise_stream_info.get_future()),
          m_aborted(false),
          m_eof(false),
          m_notifier(notifier) {}
    ~StreamReader() { stop(); }

    void start(std::string_view url, rc<PacketQueue> pkt_queue) {
        stop();

        m_url     = url;
        m_aborted = false;
        m_thread  = std::thread([this, pkt_queue] {
            DEBUG_LOG("ffmpeg read thread start");
            auto fmt_ctx = make_rc<FFmpegFormatContext>();
            read_thread(fmt_ctx, *pkt_queue);
            fmt_ctx->set_aborted(true);
            DEBUG_LOG("ffmpeg read thread end");
        });
    }

    void stop() {
        m_aborted = true;
        if (m_thread.joinable()) m_thread.join();
        m_promise_stream_info = {};
        m_future_stream_info  = m_promise_stream_info.get_future();
        DEBUG_LOG("stream reader stopped");
    }

    std::optional<StreamInfo> wait_stream_info() {
        std::shared_future future = m_future_stream_info;
        future.wait();
        try {
            return future.get();
        } catch (const std::future_error& err) {
            return std::nullopt;
        }
    }

    int best_stream(AVMediaType t) const { return m_st_idx[(int)t]; }

    void seek(float pos) { m_seek_pos = pos; }

private:
    void read_thread(rc<FFmpegFormatContext> rc_fmt_ctx, PacketQueue& pkt_queue) {
        auto&       fmt_ctx = *rc_fmt_ctx;
        FFmpegError err     = fmt_ctx.open_input(m_url.c_str());
        if (err) {
            ERROR_LOG("{}", err.what());
            return;
        }
        err = fmt_ctx.find_stream_info(NULL);
        if (err) {
            ERROR_LOG("{}", err.what());
            return;
        }

        auto max_frame_duration = (fmt_ctx->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

        // if (true) fmt_ctx.dump_format(0, m_url.c_str(), 0);

        // discard all
        for (usize i = 0; i < fmt_ctx->nb_streams; i++) {
            auto st     = fmt_ctx->streams[i];
            st->discard = AVDISCARD_ALL;
        }

        // find stream
        find_best_stream(fmt_ctx);

        auto audio_idx = best_stream(AVMEDIA_TYPE_AUDIO);
        if (audio_idx >= 0) fmt_ctx->streams[audio_idx]->discard = AVDISCARD_DEFAULT;

        {
            pkt_queue.refresh_serial();
            pkt_queue.clear();
            StreamInfo info { .ctx = rc_fmt_ctx, .st = fmt_ctx->streams, .audio_idx = audio_idx };
            m_promise_stream_info.set_value(info);
        }

        {
            notify::duration d;
            d.value = fmt_ctx->duration / (AV_TIME_BASE / 1000);
            m_notifier.send(d).wait();
        }

        Packet pkt;
        // read pkt
        for (;;) {
            if (m_aborted) break;
            // pause
            // seek req
            if (auto seek = m_seek_pos.exchange(-1.0); seek > 0) {
                i64         target = av_rescale_q((double)seek * fmt_ctx->duration,
                                          AV_TIME_BASE_Q,
                                          fmt_ctx->streams[audio_idx]->time_base);
                FFmpegError err = fmt_ctx.seek_file(audio_idx, target - 2, target, target + 2, 0);
                if (err) {
                    ERROR_LOG("{}", err);
                } else {
                    pkt_queue.clear();
                    pkt_queue.refresh_serial();
                }
            }

            if (! pkt.has_ref()) {
                FFmpegError err = fmt_ctx.read_frame(pkt.raw());
                if (err) {
                    if (err == AVERROR_EOF) {
                        ERROR_LOG("eof");
                        m_eof = true;
                        break;
                    }
                    if (fmt_ctx->pb && fmt_ctx->pb->error) {
                        ERROR_LOG("error");
                        break;
                    }
                    // wait cond
                    pkt.unref();
                    continue;
                } else {
                    m_eof = false;
                }
            } else {
                // as may not wait queue
                // sleep to avoid thread loop
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }

            auto st_idx = pkt->stream_index;
            auto st     = fmt_ctx->streams[st_idx];

            auto pkt_ref = RECORD(pkt.ref());
            if (pkt_ref) {
                // check in range
                if (st_idx == audio_idx) {
                    if (! pkt_queue.push(std::move(pkt_ref).value())) continue;
                }
            } else {
                ERROR_LOG("{}", pkt_ref.error().what());
            }
            pkt.unref();
        }
    }

    static int decode_interrupt_cb(void* self_) {
        auto self = static_cast<StreamReader*>(self_);
        return self->m_aborted;
    }

    void find_best_stream(FFmpegFormatContext& ctx) {
        m_st_idx[AVMEDIA_TYPE_VIDEO] =
            ctx.find_best_stream(AVMEDIA_TYPE_VIDEO, m_st_idx[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);

        m_st_idx[AVMEDIA_TYPE_AUDIO] = ctx.find_best_stream(AVMEDIA_TYPE_AUDIO,
                                                            m_st_idx[AVMEDIA_TYPE_AUDIO],
                                                            m_st_idx[AVMEDIA_TYPE_AUDIO],
                                                            NULL,
                                                            0);
        m_st_idx[AVMEDIA_TYPE_SUBTITLE] =
            ctx.find_best_stream(AVMEDIA_TYPE_SUBTITLE,
                                 m_st_idx[AVMEDIA_TYPE_SUBTITLE],
                                 m_st_idx[AVMEDIA_TYPE_AUDIO] >= 0 ? m_st_idx[AVMEDIA_TYPE_AUDIO]
                                                                   : m_st_idx[AVMEDIA_TYPE_VIDEO],
                                 NULL,
                                 0);
    }

private:
    std::thread             m_thread;
    std::mutex              m_mutex;
    rc<FFmpegFormatContext> m_fmt_ctx;

    std::array<int, AVMEDIA_TYPE_NB> m_st_idx;

    std::promise<AVFormatContext*> m_promise_ctx;
    std::promise<int>              m_promise_audio_idx;

    std::promise<StreamInfo>       m_promise_stream_info;
    std::shared_future<StreamInfo> m_future_stream_info;

    std::atomic<bool>  m_aborted;
    std::atomic<float> m_seek_pos;
    bool               m_eof;
    std::string        m_url;

    Notifier m_notifier;
};

} // namespace player
