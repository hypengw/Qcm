#pragma once

#include "player/player.h"

// private header
#include "context.h"
#include "stream_reader.h"
#include "audio_decoder.h"
#include "audio_device.h"

namespace player
{

class Player::Private {
public:
    C_DECLARE_PUBLIC(Player, m_q)

    Private(std::string_view name, Notifier notifier);
    ~Private();

private:
    Player*          m_q;
    Notifier m_notifier;
    rc<StreamReader> m_reader;
    up<Decoder>      m_dec;
    up<Device>       m_dev;
    rc<Context>      m_ctx;

};

} // namespace player
