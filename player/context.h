#pragma once

#include "packet_queue.h"
#include "audio_frame_queue.h"

namespace player
{

struct Context {
    static constexpr usize MaxQueueSize { 32 };

    Context()
        : audio_pkt_queue(make_rc<PacketQueue>(MaxQueueSize)),
          audio_frame_queue(make_rc<AudioFrameQueue>(MaxQueueSize)) {}

    ~Context() { set_aborted(true); }

    void set_aborted(bool v) {
        audio_pkt_queue->set_aborted(v);
        audio_frame_queue->set_aborted(v);
    }

    void clear() {
        audio_pkt_queue->clear();
        audio_frame_queue->clear();
    }

    rc<PacketQueue>     audio_pkt_queue;
    rc<AudioFrameQueue> audio_frame_queue;
};

} // namespace player
