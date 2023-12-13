#include <cubeb/cubeb.h>
#include <cassert>

#include "player/notify.h"
#include "player/player.h"
#include "stream_reader.h"
#include "player/player_p.h"

using namespace player;

Player::Player(std::string_view name, Notifier notifier): m_d(make_up<Private>(name, notifier)) {}
Player::~Player() {}

Player::Private::Private(std::string_view name, Notifier notifier)
    : m_notifier(notifier),
      m_reader(make_rc<StreamReader>(notifier)),
      m_dec(make_up<Decoder>()),
      m_dev(make_up<Device>(make_rc<DeviceContext>(name), nullptr, 2, 44100, notifier)),
      m_ctx(make_rc<Context>()) {}

Player::Private::~Private() {}

void Player::set_source(std::string_view v) {
    C_D(Player);
    stop();
    d->m_ctx->clear();

    if (v.empty()) return;

    d->m_ctx->set_aborted(false);

    d->m_dev->set_output(d->m_ctx->audio_frame_queue);
    d->m_reader->start(v, d->m_ctx->audio_pkt_queue);
    d->m_dec->start(d->m_reader, d->m_ctx->audio_pkt_queue, d->m_ctx->audio_frame_queue);

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
void Player::stop() {
    C_D(Player);
    d->m_dev->mark_dirty();

    d->m_ctx->set_aborted(true);
    d->m_reader->stop();
    d->m_dec->stop();

    d->m_notifier.send(notify::position { 0 });
    d->m_notifier.send(notify::playstate { PlayState::Stopped }).wait();
}

void Player::seek(i32 p) {
    C_D(Player);
    d->m_dev->mark_dirty();
    d->m_reader->seek(p);
    d->m_ctx->audio_pkt_queue->wake_one_pusher();
}