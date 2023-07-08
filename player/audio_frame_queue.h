#pragma once

#include <optional>
#include <mutex>
#include <condition_variable>
#include <deque>

#include "ffmpeg_error.h"
#include "audio_frame.h"
#include "audio_stream_params.h"

#include "core/core.h"
#include "core/log.h"
#include "core/queue_concurrent.h"

namespace player
{
namespace detail
{
using AudioFrameQueue = qcm::QueueWithSize<AudioFrame>;
} // namespace detail

class AudioFrameQueue : public qcm::QueueConcurrent<detail::AudioFrameQueue> {
public:
    using Self = AudioFrameQueue;
    AudioFrameQueue(usize max_size)
        : qcm::QueueConcurrent<detail::AudioFrameQueue>(max_size), m_serial(0) {}
    ~AudioFrameQueue() {}

    void prepare_item(AudioFrame& out) {
        with_lock([this, &out](lock_type&) {
            out.set_params(m_audio_params);
        });
    }

    void set_audio_params(AudioParams params) {
        with_lock([this, &params](lock_type&) {
            m_audio_params = params;
        });
    }

    bool aborted() {
        return with_lock([this](lock_type&) {
            return queue().aborted;
        });
    }

    void set_aborted(bool v) {
        with_lock([this, v](lock_type&) {
            queue().aborted = v;
            if (v) clear_wait();
        });
    }
    void refresh_serial() {
        with_lock([this](lock_type&) {
            m_serial++;
        });
    }
    auto serial() {
        return with_lock([this](lock_type&) {
            return m_serial;
        });
    }
    void set_serial(usize v) {
        return with_lock([this, v](lock_type&) {
            m_serial = v;
        });
    }

private:
    AudioParams m_audio_params;
    usize       m_serial;
};

} // namespace player
