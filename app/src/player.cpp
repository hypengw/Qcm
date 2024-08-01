#include "Qcm/player.h"
#include "Qcm/app.h"

#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>

#include <asio/error_code.hpp>
#include <asio/strand.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/as_tuple.hpp>

#include "asio_qt/qt_executor.h"
#include "core/math.h"

using namespace qcm;
using NotifyInfo = player::notify::info;

namespace
{
constexpr usize MaxChannelSize { 64 };

} // namespace

class Player::NotifyChannel : public detail::Sender<NotifyInfo> {
public:
    using channel_type = asio::experimental::concurrent_channel<asio::thread_pool::executor_type,
                                                                void(asio::error_code, NotifyInfo)>;

    NotifyChannel(rc<channel_type> ch): m_channel(ch), m_strand(ch->get_executor()) {}
    virtual ~NotifyChannel() {}

    bool try_send(NotifyInfo info) override {
        return m_channel->try_send(asio::error_code {}, info);
    }
    std::future<void> send(NotifyInfo info) override {
        return asio::co_spawn(
            m_strand,
            [info, this]() -> asio::awaitable<void> {
                co_await m_channel->async_send(asio::error_code {}, info, asio::use_awaitable);
            },
            asio::use_future);
    }
    void reset() override { m_channel->reset(); }

    auto channel() { return m_channel; }
    auto cancel() { m_channel->cancel(); }

private:
    rc<channel_type>                          m_channel;
    asio::strand<channel_type::executor_type> m_strand;
};

Player::Player(QObject* parent)
    : QObject(parent),
      m_channel(make_rc<NotifyChannel>(make_rc<NotifyChannel::channel_type>(
          Global::instance()->pool_executor(), MaxChannelSize))),
      m_end(false),
      m_position(0),
      m_duration(0),
      m_busy(false),
      m_playback_state(PlaybackState::StoppedState) {
    App::instance()->set_player_sender(sender());
    connect(this, &Player::notify, this, &Player::processNotify, Qt::QueuedConnection);

    auto channel = m_channel->channel();
    m_player     = std::make_unique<player::Player>(
        APP_NAME, player::Notifier(m_channel), Global::instance()->pool_executor());

    auto qt_exec = Global::instance()->qexecutor();
    asio::co_spawn(
        asio::strand<NotifyChannel::channel_type::executor_type>(channel->get_executor()),
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
        set_busy(true);
        QString url = m_source.toString(QUrl::PreferLocalFile | QUrl::PrettyDecoded);
        m_player->set_source(url.toStdString());

        emit sourceChanged();

        if (m_source.isLocalFile()) {
            set_cache_progress(QVector2D { 0.0f, 1.0f });
        }
    }
}

auto Player::position() const -> int { return m_position; }
auto Player::duration() const -> int { return m_duration; }
auto Player::busy() const -> bool { return m_busy; }
auto Player::playback_state() const -> Player::PlaybackState { return m_playback_state; }
auto Player::cache_progress() const -> QVector2D { return m_cache_progress; }

void Player::play() {
    set_busy(true);
    m_player->play();
}
void Player::pause() {
    set_busy(true);
    m_player->pause();
}
void Player::stop() {
    set_busy(true);
    m_player->stop();
}

void Player::set_position(int v) {
    if (m_duration > 0) {
        set_busy(true);
        m_player->seek(v + 50);
        m_channel->cancel();
        m_channel->reset();
    }
}

void Player::set_busy(bool v) {
    if (std::exchange(m_busy, v) != v) {
        emit busyChanged();
    }
}

auto Player::volume() const -> float { return m_player->volume(); }
auto Player::fadeTime() const -> u32 { return m_player->fade_time() / 1000; }

auto Player::seekable() const -> bool { return true; }
auto Player::playing() const -> bool {
    switch (playback_state()) {
    case PlayingState: return true;
    default: return false;
    }
}

auto Player::sender() const -> Sender<NotifyInfo> { return { m_channel }; }

void Player::set_volume(float val) {
    auto cur = volume();
    if (! ycore::equal_within_ulps(cur, val, 4)) {
        m_player->set_volume(val);
        volumeChanged();
    }
}
void Player::set_fadeTime(u32 val) {
    auto cur = m_player->fade_time();
    if (cur != val) {
        m_player->set_fade_time(val * 1000);
        fadeTimeChanged();
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

void Player::set_cache_progress(QVector2D val) {
    do {
        if (m_source.isLocalFile()) {
            val = { 0.0, 1.0 };
            break;
        }
        if (! ycore::equal_within_ulps(m_cache_progress.x(), val.x(), 4)) break;
        if (ycore::equal_within_ulps(1.0f, val.y(), 4)) break;
        auto delta = val.y() - m_cache_progress.y();
        if (delta < 0) break;
        if (delta > 0.05) break;
        return;
    } while (false);
    m_cache_progress = val;
    Q_EMIT cacheProgressChanged();
}

void Player::seek(double pos) {
    set_position(pos * duration());
    Q_EMIT seeked(position() * 1000.0);
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
                            },
                            [this](notify::busy b) {
                                set_busy(b.value);
                            },
                            [this](notify::cache c) {
                                set_cache_progress({ c.begin, c.end });
                            } },
               info);
}