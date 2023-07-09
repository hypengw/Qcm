#include <cubeb/cubeb.h>
#include <cassert>

#include "player/player.h"
#include "stream_reader.h"
#include "player/player_p.h"

using namespace player;

Player::Player(std::string_view name, Notifier notifier)
    : m_d(std::make_unique<Private>(name, notifier)) {}
Player::~Player() {}

Player::Private::Private(std::string_view name, Notifier notifier)
    : m_notifier(notifier),
      m_reader(std::make_shared<StreamReader>(notifier)),
      m_dec(std::make_unique<Decoder>()),
      m_dev(std::make_unique<Device>(std::make_shared<DeviceContext>(name), nullptr, 2, 44100,
                                     notifier)),
      m_ctx(std::make_shared<Context>()) {}

Player::Private::~Private() {}

void Player::set_source(std::string_view v) {
    C_D(Player);
    d->m_ctx->set_aborted(true);
    d->m_reader->stop();
    d->m_dec->stop();

    d->m_ctx->set_aborted(false);
    d->m_dev->set_output(d->m_ctx->audio_frame_queue);
    d->m_reader->start(v, d->m_ctx->audio_pkt_queue);
    d->m_dec->start(d->m_reader, d->m_ctx->audio_pkt_queue, d->m_ctx->audio_frame_queue);

    d->m_dev->mark_pos(-1);
    d->m_dev->start();
    play();
}

void Player::play() {
    C_D(Player);
    d->m_dev->set_pause(false);
}
void Player::pause() {
    C_D(Player);
    d->m_dev->set_pause(true);
}
void Player::stop() {}

void Player::seek(i32 p) {
    C_D(Player);
    d->m_dev->mark_pos(p);
    d->m_reader->seek(p);
}