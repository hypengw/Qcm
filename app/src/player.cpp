module;
#include <chrono>
#include "player/player.h"
#include "core/sender.h"

#include "Qcm/player.moc.h"

module qcm;
import :player;

namespace cppstd = rstd::cppstd;
using namespace qcm;
using NotifyInfo = player::notify::info;

namespace
{
constexpr usize MaxChannelSize { 64 };

} // namespace

class Player::NotifyChannel : public detail::Sender<NotifyInfo> {
public:
    NotifyChannel(rc<channel_type> ch): m_channel(ch), m_strand(ch->get_executor()) {}
    virtual ~NotifyChannel() {}

    bool try_send(NotifyInfo info) override {
        return m_channel->try_send(asio::error_code {}, info);
    }
    cppstd::future<void> send(NotifyInfo info) override {
        return asio::co_spawn(
            m_strand,
            [info, this]() -> asio::awaitable<void> {
                co_await m_channel->async_send(asio::error_code {}, info, use_task);
            },
            asio::use_future_t {});
    }
    void reset() override { m_channel->reset(); }

    auto channel() { return m_channel; }
    auto cancel() { m_channel->cancel(); }

private:
    rc<channel_type>                          m_channel;
    asio::strand<channel_type::executor_type> m_strand;
};

auto Player::process_msg() -> task<void> {
    auto channel = m_channel->channel();
    while (! is_end()) {
        auto [ec, info] = co_await channel->async_receive(asio::as_tuple(use_task));
        if (! ec) {
            if (const auto* pos = std::get_if<player::notify::position>(&info)) {
                set_position_raw(pos->value);
            } else {
                emit notify(info);
            }
        }
    }
    co_return;
}

Player::Player(executor_type ex, MemResourceMgr* mem, QObject* parent)
    : QObject(parent),
      m_channel(make_rc<NotifyChannel>(make_rc<channel_type>(ex, MaxChannelSize))),
      m_end(false),
      m_last_time(std::chrono::steady_clock::now()),
      m_position(0),
      m_duration(0),
      m_busy(false),
      m_playback_state(PlaybackState::StoppedState) {
    connect(this, &Player::notify, this, &Player::processNotify, Qt::QueuedConnection);

    m_player = std::make_unique<player::Player>(
        APP_NAME, player::Notifier(m_channel), ex, mem->player_mem);
}
Player::~Player() {
    if (! m_end) close();
}
void Player::close() {
    m_end = true;
    m_channel->cancel();
}

const QUrl& Player::source() const { return m_source; }
void        Player::set_source(const QUrl& v) {
    if (ycore::cmp_set(m_source, v)) {
        set_busy(true);
        // | QUrl::PrettyDecoded
        QString url = m_source.toString(QUrl::PreferLocalFile);
        m_player->set_source(url.toStdString());

        sourceChanged();

        if (m_source.isLocalFile()) {
            set_cache_progress(QVector2D { 0.0f, 1.0f });
        }
    }
}
void Player::reset_source() {
    if (! m_source.isEmpty()) {
        m_source = QUrl();
        sourceChanged();
    }
    m_player->set_source("");
}

auto Player::position() const -> int { return m_position; }
auto Player::duration() const -> int { return m_duration; }
auto Player::busy() const -> bool { return m_busy; }
auto Player::playback_state() const -> Player::PlaybackState { return m_playback_state; }
auto Player::cache_progress() const -> QVector2D { return m_cache_progress; }

void Player::toggle() {
    if (playing()) {
        m_player->pause();
    } else {
        m_player->play();
    }
}
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
    case PlaybackState::PlayingState: return true;
    default: return false;
    }
}

auto Player::sender() const -> Sender<NotifyInfo> { return { m_channel }; }

void Player::set_volume(float val) {
    auto cur = volume();
    if (! ycore::equal_within_ulps(cur, val, 4)) {
        m_player->set_volume(val);
        volumeChanged(val);
    }
}
void Player::set_fadeTime(u32 val) {
    auto cur = m_player->fade_time();
    if (cur != val) {
        m_player->set_fade_time(val * 1000);
        fadeTimeChanged(val);
    }
}

void Player::set_position_raw(int v) {
    int expected = m_position.load(std::memory_order_relaxed);
    if (m_position.compare_exchange_weak(expected, v)) {
        auto now  = cppstd::chrono::steady_clock::now();
        auto last = m_last_time.load(cppstd::memory_order::relaxed);
        if (now - last > std::chrono::milliseconds(50)) {
            m_last_time.store(now, std::memory_order_relaxed);
            emit positionChanged();
        }
    }
}

void Player::set_duration(int v) {
    if (v != std::exchange(m_duration, v)) {
        emit durationChanged();
    }
}

void Player::set_playback_state(PlaybackState v) {
    if (auto old = std::exchange(m_playback_state, v); old != v) {
        emit playbackStateChanged(old, v);
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

#include "Qcm/player.moc.cpp"
