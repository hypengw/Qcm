#pragma once

#include <atomic>
#include <cmath>
#include <cubeb/cubeb.h>
#include <optional>
#include <thread>
#include <utility>
#include <vector>
#include <mutex>
#include <algorithm>
#include <functional>
#include <memory>

#include "core/core.h"
#include "core/log.h"
#include "audio_frame_queue.h"
#include "player/notify.h"

DEFINE_CONVERT(AVSampleFormat, cubeb_sample_format) {
    switch (in) {
    case CUBEB_SAMPLE_S16LE:
    case CUBEB_SAMPLE_S16BE: out = AVSampleFormat::AV_SAMPLE_FMT_S16; break;
    case CUBEB_SAMPLE_FLOAT32LE:
    case CUBEB_SAMPLE_FLOAT32BE:
    default: out = AVSampleFormat::AV_SAMPLE_FMT_FLT;
    }
}

DEFINE_CONVERT(player::AudioParams, cubeb_stream_params) {
    out.sample_rate = in.rate;
    out.format      = convert_from<AVSampleFormat>(in.format);
    av_channel_layout_default(&out.ch_layout, in.channels);
};

namespace player
{

namespace details
{

template<typename Tout, typename Tin>
inline auto as_span(std::span<Tin> in) -> std::span<Tout> {
    _assert_((in.size() * sizeof(Tin)) % sizeof(Tout) == 0);
    return { (Tout*)in.data(), in.size() * sizeof(Tin) / sizeof(Tout) };
}

template<typename Tout, typename Tin>
inline void adjust_audio_sample(std::span<Tout> out, std::span<const Tin> in, float volume) {
    usize size = std::min(out.size(), in.size());
    for (usize i = 0; i < size; i++) {
        out[i] = in[i] * volume;
    }
}

inline void adjust_audio_sample(cubeb_sample_format, cubeb_sample_format, std::span<byte> out,
                                std::span<const byte> in, float volume) {
    adjust_audio_sample(as_span<i16>(out), as_span<const i16>(in), volume);
}

template<typename T>
inline constexpr auto fade_curve(T x, T left, T right) -> T {
    x = std::clamp<T>((x - left) / (right - left), 0.0, 1.0);
    // cubic smoothing (Catmull-Rom spline)
    // return x * x * ((T)3.0 - (T)2.0 * x);
    // lineaer
    return x;
}
} // namespace details

struct DeviceError {
    using Self = DeviceError;

    DeviceError(int e): code(e) {}
    Self& operator=(int e) noexcept {
        code = e;
        return *this;
    }
    bool operator==(int e) const noexcept { return e == code; }
    operator bool() const { return code < 0; }

    int code;
};

struct DeviceDescription {
    cubeb_devid devid;
    std::string device_id;
    std::string friendly_name;
};

class DeviceContext : NoCopy {
public:
    using up_device_collection =
        up<cubeb_device_collection, std::function<void(cubeb_device_collection*)>>;

    DeviceContext(std::string_view name): m_name(name) {
        cubeb_init(&m_cubeb_ctx, m_name.c_str(), NULL);
    }
    ~DeviceContext() { cubeb_destroy(m_cubeb_ctx); }

    std::vector<DeviceDescription> get_devices(cubeb_device_type type) {
        auto up_devices =
            up_device_collection(new cubeb_device_collection(), [this](cubeb_device_collection* p) {
                cubeb_device_collection_destroy(m_cubeb_ctx, p);
            });

        auto& devices = *up_devices;
        if (cubeb_enumerate_devices(m_cubeb_ctx, type, &devices) != CUBEB_OK) return {};

        std::span<cubeb_device_info> device_infos { devices.device, devices.count };

        std::vector<DeviceDescription> out;
        for (auto& info : device_infos) {
            DeviceDescription dp { info.devid, info.device_id, info.friendly_name };
            out.emplace_back(dp);
        }
        return out;
    }

    int get_min_latency(cubeb_stream_params& params, u32& latency) {
        return cubeb_get_min_latency(m_cubeb_ctx, &params, &latency);
    }

    rstd::Result<rc<cubeb_stream>, int> stream_init(
        char const* stream_name, cubeb_devid input_device, cubeb_stream_params* input_stream_params,
        cubeb_devid output_device, cubeb_stream_params* output_stream_params, unsigned int latency,
        cubeb_data_callback data_callback, cubeb_state_callback state_callback, void* user_ptr

    ) {
        cubeb_stream* st { nullptr };
        auto          res = cubeb_stream_init(m_cubeb_ctx,
                                     &st,
                                     stream_name,
                                     input_device,
                                     input_stream_params,
                                     output_device,
                                     output_stream_params,
                                     latency,
                                     data_callback,
                                     state_callback,
                                     user_ptr);
        if (res != CUBEB_OK) {
            return rstd::Err(res);
        } else {
            return rstd::Ok(std::shared_ptr<cubeb_stream>(st, &cubeb_stream_destroy));
        }
    }

private:
    std::string m_name;
    cubeb*      m_cubeb_ctx;
};

class Device {
public:
    using Self                       = Device;
    static constexpr u32 kBlockCount = 24;

    Device(rc<DeviceContext> ctx, cubeb_devid devid, i32 channels, i32 samplerate,
           Notifier notifier)

        : m_ctx(ctx),
          m_stream(nullptr),
          m_channels(channels),
          m_volume(1.0f),
          m_volume_gain(1.0f),
          m_paused(false),
          m_mark_pos(0),
          m_mark_serial(-1),
          m_notifier(notifier) {
        cubeb_stream_params output_params;

        output_params.rate     = samplerate;
        output_params.channels = channels;
        output_params.format   = CUBEB_SAMPLE_S16NE;
        output_params.prefs    = CUBEB_STREAM_PREF_NONE;
        output_params.layout   = CUBEB_LAYOUT_UNDEFINED;

        u32 latency = 1;
        m_ctx->get_min_latency(output_params, latency);

        // m_buffer.reserve(m_bytes_per_block * kBlockCount);

        m_stream = m_ctx
                       ->stream_init("Cubeb output",
                                     nullptr,
                                     nullptr,
                                     devid,
                                     &output_params,
                                     latency,
                                     Self::data_cb,
                                     Self::state_cb,
                                     this)
                       .expect("can't initialize cubeb device");

        m_audio_params = convert_from<AudioParams>(output_params);
    };
    ~Device() {
        if (m_thread.joinable()) m_thread.join();
    };

    void start() {
        fade_reset();
        cubeb_stream_start(m_stream.get());
    }
    void stop() {
        fade_reset();
        cubeb_stream_stop(m_stream.get());
    }

    auto volume() const noexcept -> float { return m_volume; }
    auto volume_gain() const noexcept -> float { return m_volume_gain; }
    void set_volume(float v) noexcept {
        m_volume = v;
        if (0) {
            // db
            float min_db  = -60.0f;
            float max_db  = 0.0f;
            float db      = min_db + m_volume * (max_db - min_db);
            m_volume_gain = std::powf(10.0f, db / 20.0f);
        } else {
            // use x^3
            m_volume_gain = std::powf(m_volume, 3);
        }
    }

    bool set_output_volume(float v) {
        return CUBEB_OK == cubeb_stream_set_volume(m_stream.get(), v);
    }

    void set_output(rc<AudioFrameQueue> in) {
        m_output_queue = in;
        m_output_queue->set_audio_params(m_audio_params);
    }

    bool fading() const { return m_fade.fading(); }
    void fade_reset() { return m_fade.reset(); }
    auto fade_duration() const { return m_fade.duration; }
    void set_fade_duration(u32 val) { m_fade.duration = val; }

    bool paused() const { return m_paused; }
    void set_pause(bool v) {
        m_paused = v;
        fade_reset();

        notify::playstate s;
        s.value = v ? PlayState::Paused : PlayState::Playing;
        m_notifier.send(s).wait();
    }

    void mark_dirty() {
        if (m_output_queue) m_mark_serial = m_output_queue->serial();
    }

    bool dirty() const { return m_mark_serial == m_output_queue->serial(); }

private:
    struct Frame {
        static rstd::Option<Frame> from(rstd::Option<AudioFrame> f) {
            return f.map([](AudioFrame&& f_) {
                if (f_.eof()) {
                    return Frame { .frame = std::move(f_), .data = {} };
                } else {
                    _assert_rel_(! f_.is_planar());
                    auto data     = f_.channel_data(0);
                    auto duration = (float)f_.ff->nb_samples / f_.ff->sample_rate;
                    auto pts      = duration_cast<milliseconds>(f_.pts_duration()).count();
                    return Frame { .frame     = std::move(f_),
                                   .data      = data,
                                   .full_size = data.size(),
                                   .duration  = duration,
                                   .timestamp = pts };
                }
            });
        }
        AudioFrame            frame;
        std::span<const byte> data;
        usize                 full_size { 0 };
        float                 duration { 0 };
        i64                   timestamp { 0 }; // milli
        bool                  notified { false };
    };

    void notify(Frame& frame) {
        if (frame.frame.eof()) {
            notify::playstate p { PlayState::Stopped };
            m_notifier.send(p);
        } else {
            notify::position pos;
            pos.value = frame.timestamp;
            if (! dirty()) {
                m_notifier.try_send(pos);
            }
        }
    }

    // todo : destroy order, make self unvalid
    static long data_cb(cubeb_stream*, void* user, const void*, void* outputbuffer, long nframes) {
        auto*      self = (Self*)user;
        const auto size =
            (usize)nframes * self->m_channels * self->m_audio_params.bytes_per_sample();
        std::span<byte> output { (byte*)outputbuffer, size };

        if (! self->dirty()) {
            auto& frame = self->m_cached_frame;
            if (! frame) {
                frame = Frame::from(rstd::into(self->m_output_queue->try_pop()));
            }

            while (frame && (! self->paused() || self->fading())) {
                if (! frame->notified) self->notify(*frame);

                auto copied = std::min(output.size(), frame->data.size());
                details::adjust_audio_sample(CUBEB_SAMPLE_S16NE,
                                             CUBEB_SAMPLE_S16NE,
                                             output.subspan(0, copied),
                                             frame->data.subspan(0, copied),
                                             self->volume_gain() * self->fade_factor());
                // std::copy_n(frame->data.begin(), copied, output.begin());
                output      = output.subspan(copied);
                frame->data = frame->data.subspan(copied);

                self->fade_elapse(frame->duration * ((float)copied / frame->full_size));

                if (frame->data.empty()) {
                    frame = Frame::from(rstd::into(self->m_output_queue->try_pop()));
                }
                if (output.empty()) break;
            };
        } else {
            self->m_output_queue->try_pop();
            self->m_cached_frame = None();
        }

        // silence
        std::fill(output.begin(), output.end(), byte {});
        return nframes;
    }

    static void state_cb(cubeb_stream* stream, void*, cubeb_state state) {
        if (! stream) return;
        switch (state) {
        case CUBEB_STATE_STARTED: LOG_INFO("stream started"); break;
        case CUBEB_STATE_STOPPED: LOG_INFO("stream stopped"); break;
        case CUBEB_STATE_DRAINED: LOG_INFO("stream drained"); break;
        default: LOG_INFO("unknown stream state {}", (int)state);
        }
    }

    void fade_elapse(float t) { m_fade.elapse(t); }
    auto fade_factor() const -> float { return m_fade.factor(m_paused); }

private:
    std::thread m_thread;

    rc<DeviceContext> m_ctx;
    rc<cubeb_stream>  m_stream;

    i32                m_channels;
    std::atomic<float> m_volume;
    std::atomic<float> m_volume_gain;
    struct FadeInfo {
        u32                       duration { 500 * 1000 }; // mill
        std::atomic<float>        elapsed_time { 0 };
        mutable std::atomic<bool> end { false };

        template<typename T>
        auto elapsed_micro() const -> T {
            return std::min<float>(elapsed_time, 1.0f) * 1000.0 * 1000.0;
        }

        auto fading() const -> bool {
            if (end || duration == 0) return false;
            bool fading = elapsed_micro<u32>() < duration;
            end         = ! fading;
            return fading;
        }
        void reset() {
            elapsed_time = 0.0;
            end          = false;
        }
        auto factor(bool paused) const -> float {
            return paused
                       ? ((end || duration == 0)
                              ? 0.0
                              : details::fade_curve<float>(
                                    -elapsed_micro<float>(), -(float)duration, 0.0f))
                       : ((end || duration == 0)
                              ? 1.0
                              : details::fade_curve<float>(elapsed_micro<float>(), 0.0f, duration));
        }
        void elapse(float t) {
            if (fading()) {
                elapsed_time += t;
            }
        }
    };
    FadeInfo m_fade;

    rstd::Option<Frame> m_cached_frame;
    std::atomic<bool>   m_paused;
    std::atomic<i32>    m_mark_pos;
    std::atomic<usize>  m_mark_serial;

    AudioParams         m_audio_params;
    rc<AudioFrameQueue> m_output_queue;
    Notifier            m_notifier;
    // i64                 m_last_pts;
};

} // namespace player
