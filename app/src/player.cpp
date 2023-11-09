#include "Qcm/player.h"
#include "Qcm/app.h"

#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>

#include "asio/error_code.hpp"
#include "asio/strand.hpp"
#include "asio/thread_pool.hpp"
#include "asio/use_awaitable.hpp"
#include "asio_qt/qt_executor.h"

using namespace qcm;
using NotifyInfo = player::notify::info;

namespace
{
constexpr usize MaxChannelSize { 64 };

class NotifierInner : public detail::Sender<NotifyInfo> {
public:
    using channel_type = Player::channel_type;

    NotifierInner(rc<channel_type> ch): channel(ch), strand(ch->get_executor()) {}
    virtual ~NotifierInner() {}

    bool try_send(NotifyInfo info) override { return channel->try_send(asio::error_code {}, info); }
    std::future<void> send(NotifyInfo info) override {
        return asio::co_spawn(
            strand,
            [info, this]() -> asio::awaitable<void> {
                co_await channel->async_send(asio::error_code {}, info, asio::use_awaitable);
            },
            asio::use_future);
    }
    void reset() override { channel.reset(); }

    rc<channel_type>                          channel;
    asio::strand<channel_type::executor_type> strand;
};

} // namespace

Player::Player(QObject* parent)
    : QObject(parent),
      m_channel(make_rc<NotifierInner::channel_type>(App::instance()->get_pool_executor(),
                                                     MaxChannelSize)),
      m_end(false),
      m_position(0),
      m_duration(0),
      m_playback_state(PlaybackState::StoppedState) {
    connect(this, &Player::notify, this, &Player::processNotify, Qt::QueuedConnection);

    auto channel       = m_channel;
    auto notifer_inner = make_rc<NotifierInner>(channel);
    m_player = std::make_unique<player::Player>(APP_NAME, player::Notifier(notifer_inner));

    auto qt_exec = App::instance()->get_executor();
    asio::co_spawn(
        asio::strand<NotifierInner::channel_type::executor_type>(channel->get_executor()),
        [this, channel]() -> asio::awaitable<void> {
            while (! m_end) {
                auto [ec, info] =
                    co_await channel->async_receive(asio::as_tuple(asio::use_awaitable));
                if (! ec) {
                    if (const auto* pos = std::get_if<player::notify::position>(&info)) {
                        set_position_raw(pos->value);
                    } else {
                        emit notify(info);
                    }
                }
            }
            co_return;
        },
        asio::detached);
}
Player::~Player() {
    m_end = true;
    m_channel->cancel();
}

const QUrl& Player::source() const { return m_source; }
void        Player::set_source(const QUrl& v) {
    if (std::exchange(m_source, v) != v) {
        emit sourceChanged();

        QString url = m_source.toString(QUrl::PreferLocalFile | QUrl::PrettyDecoded);
        m_player->set_source(url.toStdString());
    }
}

int                   Player::position() const { return m_position; }
int                   Player::duration() const { return m_duration; }
Player::PlaybackState Player::playbackState() const { return m_playback_state; }

void Player::play() { m_player->play(); }
void Player::pause() { m_player->pause(); }
void Player::stop() { m_player->stop(); }

void Player::set_position(int v) {
    if (m_duration > 0) {
        m_player->seek(v + 50);
        m_channel->cancel();
        m_channel->reset();
    }
}

void Player::set_position_raw(int v) {
    int expected = m_position.load();
    if (m_position.compare_exchange_weak(expected, v)) {
        emit positionChanged();
    }
}

void Player::set_duration(int v) {
    if (v != std::exchange(m_duration, v)) {
        emit durationChanged();
    }
}

void Player::set_playback_state(PlaybackState v) {
    if (v != std::exchange(m_playback_state, v)) {
        emit playbackStateChanged();
    }
}

void Player::processNotify(NotifyInfo info) {
    using namespace player;
    std::visit(overloaded { [](notify::position) {
                           },
                            [this](notify::duration d) {
                                set_duration(d.value);
                            },
                            [this](notify::playstate s) {
                                set_playback_state((PlaybackState)(int)s.value);
                            } },
               info);
}