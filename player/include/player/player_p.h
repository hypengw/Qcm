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
class Player::Private {
public:
    C_DECLARE_PUBLIC(Player, m_q)
    using action_receiver_type = rstd::async::CompletionQueue<action::info>;
    using action_sender_type   = rstd::async::CompletionQueueHandle<action::info>;
    using action_channel_type  = rstd::tuple<action_receiver_type, action_sender_type>;

    Private(std::string_view name, Notifier notifier,
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

    action_channel_type m_action_channel;
    std::atomic<u32>    m_action_id;

    rc<StreamReader> m_reader;
    up<Decoder>      m_dec;
    up<Device>       m_dev;
    rc<Context>      m_ctx;
};

} // namespace player
