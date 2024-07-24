#include <cubeb/cubeb.h>
#include <cassert>

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/as_tuple.hpp>

#include "player/notify.h"
#include "player/player.h"
#include "stream_reader.h"
#include "player/player_p.h"

using namespace player;

Player::Player(std::string_view name, Notifier notifier, executor_type exc)
    : m_d(make_up<Private>(name, notifier, exc)) {}
Player::~Player() {}

Player::Private::Private(std::string_view name, Notifier notifier, executor_type exc)
    : m_notifier(notifier),
      m_action_channel(make_rc<action_channel_type>(exc, 64)),
      m_action_id(0),
      m_end(false),
      m_reader(make_rc<StreamReader>(notifier)),
      m_dec(make_up<Decoder>()),
      m_dev(make_up<Device>(make_rc<DeviceContext>(name), nullptr, 2, 44100, notifier)),
      m_ctx(make_rc<Context>()) {
    auto channel = m_action_channel;
    asio::co_spawn(
        asio::strand<action_channel_type::executor_type>(channel->get_executor()),
        [this, channel]() -> asio::awaitable<void> {
            while (! m_end) {
                auto [ec, info] =
                    co_await channel->async_receive(asio::as_tuple(asio::use_awaitable));
                if (! ec) {
                    this->m_notifier.try_send(notify::busy { true });
                    u32 id = std::visit(overloaded {
                                            [this](action::play a) {
                                                this->play();
                                                return a.id;
                                            },
                                            [this](action::pause a) {
                                                this->pause();
                                                return a.id;
                                            },
                                            [this](action::stop a) {
                                                this->stop();
                                                return a.id;
                                            },
                                            [this](action::seek a) {
                                                this->seek(a.value);
                                                return a.id;
                                            },
                                            [this](action::source a) {
                                                this->set_source(a.value);
                                                return a.id;
                                            },
                                        },
                                        info);
                    if (m_action_id == id + 1) {
                        this->m_notifier.try_send(notify::busy { false });
                    }
                }
            }
            co_return;
        },
        asio::detached);
}

Player::Private::~Private() {
    m_end = true;
    m_action_channel->cancel();
}

void Player::Private::set_source(std::string_view v) {
    stop();
    m_ctx->clear();

    if (v.empty()) return;

    m_ctx->set_aborted(false);

    m_dev->set_output(m_ctx->audio_frame_queue);
    m_reader->start(v, m_ctx->audio_pkt_queue);
    m_dec->start(m_reader, m_ctx->audio_pkt_queue, m_ctx->audio_frame_queue);

    m_dev->start();
    play();
}

void Player::Private::play() { m_dev->set_pause(false); }
void Player::Private::pause() { m_dev->set_pause(true); }
void Player::Private::stop() {
    m_dev->mark_dirty();

    m_ctx->set_aborted(true);
    m_reader->stop();
    m_dec->stop();

    m_notifier.send(notify::position { 0 });
    m_notifier.send(notify::playstate { PlayState::Stopped }).wait();
}

void Player::Private::seek(i32 p) {
    m_dev->mark_dirty();
    m_reader->seek(p);
    m_ctx->audio_pkt_queue->wake_one_pusher();
}

void Player::set_source(std::string_view v) {
    C_D(Player);
    d->m_action_channel->try_send(asio::error_code {},
                                  action::source { std::string(v), d->m_action_id++ });
}

void Player::play() {
    C_D(Player);
    d->m_action_channel->try_send(asio::error_code {}, action::play { d->m_action_id++ });
}
void Player::pause() {
    C_D(Player);
    d->m_action_channel->try_send(asio::error_code {}, action::pause { d->m_action_id++ });
}
void Player::stop() {
    C_D(Player);
    d->m_action_channel->try_send(asio::error_code {}, action::stop { d->m_action_id++ });
}
void Player::seek(i32 p) {
    C_D(Player);
    d->m_action_channel->try_send(asio::error_code {}, action::seek { p, d->m_action_id++ });
}

auto Player::volume() const -> float {
    C_D(const Player);
    return d->m_dev->volume();
}
void Player::set_volume(float val) {
    C_D(Player);
    d->m_dev->set_volume(val);
}
auto Player::fade_time() const -> u32 {
    C_D(const Player);
    return d->m_dev->fade_duration();
}
void Player::set_fade_time(u32 val) {
    C_D(Player);
    d->m_dev->set_fade_duration(val);
}