#pragma once

#include "player/player.h"

// private header
#include "context.h"
#include "stream_reader.h"
#include "audio_decoder.h"
#include "audio_device.h"

namespace player
{
namespace action
{
template<typename T>
struct base {
    T   value {};
    u32 id {};
};
template<>
struct base<void> {
    u32 id {};
};

struct source : base<std::string> {};
struct play : base<void> {};
struct pause : base<void> {};
struct stop : base<void> {};
struct seek : base<i32> {};

using info = std::variant<source, play, pause, stop, seek>;
} // namespace action
//
using Action = qcm::Sender<action::info>;

class Player::Private {
public:
    C_DECLARE_PUBLIC(Player, m_q)
    using action_channel_type =
        asio::experimental::concurrent_channel<asio::thread_pool::executor_type,
                                               void(asio::error_code, action::info)>;

    Private(std::string_view name, Notifier notifier, asio::thread_pool::executor_type exc,
            std::pmr::memory_resource* mem);
    ~Private();

    void play();
    void pause();
    void stop();
    void seek(i32);

    void set_source(std::string_view);

private:
    Player*  m_q;
    Notifier m_notifier;

    rc<action_channel_type> m_action_channel;
    std::atomic<u32>        m_action_id;
    bool                    m_end;

    rc<StreamReader> m_reader;
    up<Decoder>      m_dec;
    up<Device>       m_dev;
    rc<Context>      m_ctx;
};

} // namespace player
