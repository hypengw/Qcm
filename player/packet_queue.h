#pragma once

#include <utility>
#include <deque>

extern "C" {
#include <libavformat/avformat.h>
}
#include "core/queue_concurrent.h"
#include "ffmpeg_error.h"

namespace player
{

struct Packet : NoCopy {
public:
    using Self = Packet;

    Packet(): m_pkt(av_packet_alloc()) {}
    ~Packet() { av_packet_free(&m_pkt); }
    Packet(Self&& o): Packet() { *this = std::move(o); }
    Self& operator=(Self&& o) noexcept {
        std::swap(m_pkt, o.m_pkt);
        return *this;
    }

    static FFmpegResult<Self> ref(const AVPacket* in) noexcept {
        Self        out;
        FFmpegError err = av_packet_ref(out.m_pkt, in);
        if (err) return UNEXPECTED(err);
        return out;
    }

    auto operator->() const { return m_pkt; }

    void unref() { av_packet_unref(m_pkt); }

    Self move() {
        Self out;
        av_packet_move_ref(out.m_pkt, m_pkt);
        return out;
    }

    bool has_ref() const { return m_pkt->buf != NULL; }

    auto ref() const { return Self::ref(m_pkt); }

    auto raw() const { return m_pkt; }
    auto praw() { return &m_pkt; }

    void set_eof() { m_pkt->pts = -2; }
    bool eof() const { return m_pkt->pts == -2; }

private:
    AVPacket* m_pkt;
};

namespace detail
{
using PacketQueue = qcm::QueueWithSize<Packet>;
} // namespace detail

class PacketQueue : public qcm::QueueConcurrent<detail::PacketQueue> {
public:
    PacketQueue(usize max_size): qcm::QueueConcurrent<detail::PacketQueue>(max_size), m_serial(0) {}

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
    usize m_serial;
};

} // namespace player
