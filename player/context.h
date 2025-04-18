#pragma once

#include "packet_queue.h"
#include "audio_frame_queue.h"

namespace player
{

struct Context {
    Context()
        : audio_pkt_queue(make_rc<PacketQueue>(2 * 1024 * 1024)), // 2 MB
          audio_frame_queue(make_rc<AudioFrameQueue>(32)) {}

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
