#pragma once

#include "ffmpeg_error.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "core/core.h"
#include "ffmpeg_frame.h"
#include "stream_reader.h"
#include "audio_frame_queue.h"
#include "resampler.h"

namespace player
{

class Decoder : NoCopy {
public:
    using StreamInfo = StreamReader::StreamInfo;
    Decoder() {}
    ~Decoder() { stop(); }

    using FFmpegDecoderContext = up<AVCodecContext, decltype([](AVCodecContext* ctx) {
                                        avcodec_free_context(&ctx);
                                    })>;

    void start(weak<StreamReader> reader, rc<PacketQueue> in_queue, rc<AudioFrameQueue> out_queue) {
        m_thread = std::thread([this, reader, in_queue, out_queue] {
            FFmpegDecoderContext dec_ctx(avcodec_alloc_context3(NULL));

            StreamInfo st_info;
            auto*      ctx = dec_ctx.get();
            if (wait_stream_info(st_info, reader) && open_codec(ctx, st_info)) {
                decoder_thread(ctx, *in_queue, *out_queue);
            }
        });
    }

    void stop() {
        if (m_thread.joinable()) m_thread.join();
        DEBUG_LOG("decoder stopped");
    }

    void decoder_thread(AVCodecContext* ctx, PacketQueue& pkt_queue, AudioFrameQueue& queue) {
        switch (ctx->codec_type) {
        case AVMEDIA_TYPE_AUDIO: audio_thread(ctx, pkt_queue, queue); break;
        default: break;
        }
    }

private:
    bool wait_stream_info(StreamInfo& st_info, weak<StreamReader> weak_reader) {
        do {
            auto reader = weak_reader.lock();
            if (! reader) break;
            auto st_info_opt = reader->wait_stream_info();
            if (! st_info_opt) break;
            st_info = st_info_opt.value();
            return true;
        } while (false);
        return false;
    }

    const AVCodec* open_codec(AVCodecContext* ctx, StreamInfo& st_info) {
        auto idx = st_info.audio_idx;
        auto st  = st_info.st[idx];

        FFmpegError err = avcodec_parameters_to_context(ctx, st->codecpar);
        if (err) {
            ERROR_LOG("{}", err.what());
            return nullptr;
        }

        ctx->pkt_timebase = st->time_base;
        auto codec        = avcodec_find_decoder(ctx->codec_id);
        if (! codec) {
            ERROR_LOG("can not find decoder for {}", avcodec_get_name(ctx->codec_id));
            return nullptr;
        }
        ctx->codec_id = codec->id;
        if (err = avcodec_open2(ctx, codec, NULL); err) {
            ERROR_LOG("{}", err.what());
            return nullptr;
        }
        return codec;
    }

    void audio_thread(AVCodecContext* ctx, PacketQueue& pkt_queue, AudioFrameQueue& queue) {
        FFmpegFrame   frame;
        FFmpegError   err       = AVERROR(EAGAIN);
        up<Resampler> resampler = make_up<Resampler>();
        do {
            if (err) {
                ERROR_LOG("{}", err.what());
            }
            if (pkt_queue.serial() != queue.serial()) {
                queue.clear();
                queue.set_serial(pkt_queue.serial());
            }

            err = decode_frame(ctx, pkt_queue, frame.raw());
            if (err) continue;

            auto out_frame = AudioFrame();
            out_frame.ff.copy_props(frame);
            queue.prepare_item(out_frame);
            auto in_frame = AudioFrame::from(frame);
            // resampler
            {
                err = resampler->resampler(out_frame, in_frame.value());
                if (err == AVERROR_INPUT_CHANGED || err == AVERROR_OUTPUT_CHANGED) {
                    resampler->close();
                    err = resampler->resampler(out_frame, in_frame.value());
                }
            }
            if (err) continue;
            queue.push(std::move(out_frame));
        } while ((! err || err == AVERROR(EAGAIN) || err == AVERROR_EOF));
        if (err && err != FFmpegError::EABORTED) {
            ERROR_LOG("{}", err.what());
        }
        return;
    }

    FFmpegError decode_frame(AVCodecContext* ctx, PacketQueue& pkt_queue, AVFrame* frame) {
        FFmpegError err = AVERROR(EAGAIN);
        for (;;) {
            do {
                err              = avcodec_receive_frame(ctx, frame);
                frame->time_base = ctx->pkt_timebase;
                if (err == AVERROR_EOF) {
                    avcodec_flush_buffers(ctx);
                    return err;
                }
                if (! err) return err;
            } while (err != AVERROR(EAGAIN));

            do {
                if (auto pkt = pkt_queue.pop()) {
                    err = avcodec_send_packet(ctx, pkt->raw());
                    if (! err)
                        break;
                    else {
                        ERROR_LOG("{}", err.what());
                    }
                } else if (pkt_queue.aborted()) {
                    err = FFmpegError::EABORTED;
                    return err;
                }
            } while (true);
        }
        return err;

        // seek or switch
        // avcodec_flush_buffers();
    }

private:
    std::thread m_thread;
};

} // namespace player