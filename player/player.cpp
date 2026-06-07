#include <cassert>
#include <cubeb/cubeb.h>

#include "player/player.h"
#include "player/player_p.h"
#include "stream_reader.h"

using namespace player;
using namespace rstd::literals;

Player::Player(std::string_view name, Notifier notifier, std::pmr::memory_resource* mem)
    : m_d(make_up<Private>(name, std::move(notifier), mem)) {}

Player::~Player() { close(); }

Player::Private::Private(std::string_view name, Notifier notifier,
                         std::pmr::memory_resource* mem)
    : m_notifier(std::move(notifier)),
      m_action_channel(rstd::move(action_receiver_type::make()).unwrap_unchecked()),
      m_action_id(0),
      m_reader(make_rc<StreamReader>(m_notifier)),
      m_dec(make_up<Decoder>()),
      m_dev(make_up<Device>(DeviceContext::make(name).expect("can't initialize cubeb context"_str),
                            nullptr,
                            2,
                            44100,
                            m_notifier)),
      m_ctx(make_rc<Context>(mem)) {}

Player::Private::~Private() {
    m_action_channel.get<1>().close();
    stop();
}

auto Player::process_actions() -> rstd::async::coro<void> {
    C_D(Player);
    auto& receiver = d->m_action_channel.get<0>();

    for (;;) {
        auto next = co_await receiver.next();
        if (next.is_err()) co_return;

        auto item = rstd::move(next).unwrap_unchecked();
        if (item.is_none()) co_return;

        d->m_notifier.try_send(notify::busy { true });
        auto action_id = std::visit(overloaded {
                                        [d](action::play action) {
                                            d->play();
                                            return action.id;
                                        },
                                        [d](action::pause action) {
                                            d->pause();
                                            return action.id;
                                        },
                                        [d](action::stop action) {
                                            d->stop();
                                            return action.id;
                                        },
                                        [d](action::seek action) {
                                            d->seek(action.value);
                                            return action.id;
                                        },
                                        [d](action::source action) {
                                            d->set_source(action.value);
                                            return action.id;
                                        },
                                    },
                                    rstd::move(item).unwrap_unchecked());
        if (d->m_action_id.load() == action_id + 1) {
            d->m_notifier.try_send(notify::busy { false });
        }
    }
}

void Player::close() {
    C_D(Player);
    d->m_action_channel.get<1>().close();
}

void Player::Private::set_source(std::string_view value) {
    stop();
    m_notifier.advance_epoch();
    m_ctx->clear();

    if (value.empty()) return;

    m_ctx->set_aborted(false);
    m_dev->set_output(m_ctx->audio_frame_queue);
    m_reader->start(value, m_ctx->audio_pkt_queue);
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
    m_notifier.send(notify::playstate { PlayState::Stopped });
}

void Player::Private::seek(i32 position) {
    m_dev->mark_dirty();
    m_notifier.advance_epoch();
    m_reader->seek(position);
    m_ctx->audio_pkt_queue->wake_one_pusher();
}

void Player::set_source(std::string_view value) {
    C_D(Player);
    (void)d->m_action_channel.get<1>().push(
        action::source { std::string(value), d->m_action_id++ });
}

void Player::play() {
    C_D(Player);
    (void)d->m_action_channel.get<1>().push(action::play { d->m_action_id++ });
}

void Player::pause() {
    C_D(Player);
    (void)d->m_action_channel.get<1>().push(action::pause { d->m_action_id++ });
}

void Player::stop() {
    C_D(Player);
    (void)d->m_action_channel.get<1>().push(action::stop { d->m_action_id++ });
}

void Player::seek(i32 position) {
    C_D(Player);
    (void)d->m_action_channel.get<1>().push(action::seek { position, d->m_action_id++ });
}

auto Player::volume() const -> float {
    C_D(const Player);
    return d->m_dev->volume();
}

void Player::set_volume(float value) {
    C_D(Player);
    d->m_dev->set_volume(value);
}

auto Player::fade_time() const -> u32 {
    C_D(const Player);
    return d->m_dev->fade_duration();
}

void Player::set_fade_time(u32 value) {
    C_D(Player);
    d->m_dev->set_fade_duration(value);
}
