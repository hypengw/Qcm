module;
#include <chrono>
#include <mutex>
#include <optional>

#include "player/player.h"
#include "Qcm/player.moc.h"

module qcm;
import :player;

using namespace qcm;
using namespace qextra::prelude;
using NotifyInfo = player::notify::info;

class Player::NotifyChannel : public player::detail::NotifySink {
public:
    struct Notification {
        u64        epoch;
        NotifyInfo info;
    };

    using Receiver = rstd::async::CompletionQueue<Notification>;
    using Sender   = rstd::async::CompletionQueueHandle<Notification>;

    NotifyChannel(Receiver receiver, Sender sender)
        : m_receiver(rstd::move(receiver)), m_sender(rstd::move(sender)) {}

    static auto make() -> rc<NotifyChannel> {
        auto pair = rstd::move(Receiver::make()).unwrap_unchecked();
        return make_rc<NotifyChannel>(rstd::move(pair.get<0>()),
                                      rstd::move(pair.get<1>()));
    }

    bool send(NotifyInfo info) override {
        auto notification = Notification { m_epoch.load(), std::move(info) };
        if (std::holds_alternative<player::notify::position>(notification.info)) {
            return enqueue_latest(std::move(notification), m_latest_position, m_position_queued);
        }
        if (std::holds_alternative<player::notify::cache>(notification.info)) {
            return enqueue_latest(std::move(notification), m_latest_cache, m_cache_queued);
        }
        return m_sender.push(rstd::move(notification)).is_ok();
    }

    auto advance_epoch() -> u64 override {
        std::lock_guard lock(m_latest_mutex);
        m_latest_position.reset();
        m_latest_cache.reset();
        return m_epoch.fetch_add(1) + 1;
    }

    auto epoch() const -> u64 { return m_epoch.load(); }
    auto next() { return m_receiver.next(); }
    void close() { m_sender.close(); }

    auto resolve(Notification notification) -> std::optional<Notification> {
        if (std::holds_alternative<player::notify::position>(notification.info)) {
            return take_latest(m_latest_position, m_position_queued);
        }
        if (std::holds_alternative<player::notify::cache>(notification.info)) {
            return take_latest(m_latest_cache, m_cache_queued);
        }
        return notification;
    }

private:
    bool enqueue_latest(Notification notification, std::optional<Notification>& latest,
                        bool& queued) {
        std::lock_guard lock(m_latest_mutex);
        latest = notification;
        if (queued) return true;

        queued = true;
        auto result = m_sender.push(rstd::move(notification));
        if (result.is_err()) {
            latest.reset();
            queued = false;
            return false;
        }
        return true;
    }

    auto take_latest(std::optional<Notification>& latest, bool& queued)
        -> std::optional<Notification> {
        std::lock_guard lock(m_latest_mutex);
        auto result = std::move(latest);
        latest.reset();
        queued = false;
        return result;
    }

    Receiver m_receiver;
    Sender   m_sender;

    std::atomic<u64> m_epoch { 0 };
    std::mutex       m_latest_mutex;
    std::optional<Notification> m_latest_position;
    std::optional<Notification> m_latest_cache;
    bool                        m_position_queued { false };
    bool                        m_cache_queued { false };
};

Player::Player(MemResourceMgr* memory, QObject* parent)
    : QObject(parent),
      m_channel(NotifyChannel::make()),
      m_player(make_rc<player::Player>(APP_NAME,
                                       player::Notifier(m_channel),
                                       memory->player_mem)),
      m_action_runner(new QAsyncResult(this)),
      m_notify_runner(new QAsyncResult(this)),
      m_closed(false),
      m_last_time(std::chrono::steady_clock::now()),
      m_position(0),
      m_duration(0),
      m_busy(false),
      m_playback_state(PlaybackState::StoppedState) {
    m_action_runner->setForwardError(false);
    m_notify_runner->setForwardError(false);

    m_action_runner->spawn([player = m_player]() -> task<void> {
        co_await player->process_actions();
    });

    auto self    = QPointer<Player> { this };
    auto channel = m_channel;
    m_notify_runner->spawn([self, channel]() -> task<void> {
        for (;;) {
            auto next = co_await channel->next();
            if (next.is_err()) co_return;

            auto item = rstd::move(next).unwrap_unchecked();
            if (item.is_none()) co_return;

            auto resolved = channel->resolve(rstd::move(item).unwrap_unchecked());
            if (! resolved || resolved->epoch != channel->epoch()) continue;

            if (! co_await QAsyncResult::qexecutor()) co_return;
            if (! self) co_return;
            if (resolved->epoch != channel->epoch()) continue;

            auto info = rstd::move(resolved->info);
            if (const auto* position = std::get_if<player::notify::position>(&info)) {
                self->set_position_raw(static_cast<int>(position->value));
            } else {
                self->processNotify(info);
                Q_EMIT self->notify(rstd::move(info));
            }
        }
    });
}

Player::~Player() { close(); }

void Player::close() {
    if (std::exchange(m_closed, true)) return;
    m_player->close();
    m_channel->close();
    m_action_runner->cancel();
    m_notify_runner->cancel();
    m_player.reset();
}

const QUrl& Player::source() const { return m_source; }

void Player::set_source(const QUrl& value) {
    if (ycore::cmp_set(m_source, value)) {
        set_busy(true);
        auto url = m_source.toString(QUrl::PreferLocalFile);
        m_player->set_source(url.toStdString());
        sourceChanged();

        if (m_source.isLocalFile()) set_cache_progress(QVector2D { 0.0f, 1.0f });
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

void Player::set_position(int value) {
    if (m_duration > 0) {
        set_busy(true);
        m_player->seek(value + 50);
    }
}

void Player::set_busy(bool value) {
    if (std::exchange(m_busy, value) != value) Q_EMIT busyChanged();
}

auto Player::volume() const -> float { return m_player->volume(); }
auto Player::fadeTime() const -> u32 { return m_player->fade_time() / 1000; }

auto Player::seekable() const -> bool { return true; }

auto Player::playing() const -> bool {
    return playback_state() == PlaybackState::PlayingState;
}

auto Player::sender() const -> player::Notifier { return player::Notifier(m_channel); }

void Player::set_volume(float value) {
    auto current = volume();
    if (! ycore::equal_within_ulps(current, value, 4)) {
        m_player->set_volume(value);
        Q_EMIT volumeChanged(value);
    }
}

void Player::set_fadeTime(u32 value) {
    auto current = m_player->fade_time();
    if (current != value) {
        m_player->set_fade_time(value * 1000);
        Q_EMIT fadeTimeChanged(value);
    }
}

void Player::set_position_raw(int value) {
    int expected = m_position.load(std::memory_order_relaxed);
    if (m_position.compare_exchange_weak(expected, value)) {
        auto now  = std::chrono::steady_clock::now();
        auto last = m_last_time.load(std::memory_order_relaxed);
        if (now - last > std::chrono::milliseconds(50)) {
            m_last_time.store(now, std::memory_order_relaxed);
            Q_EMIT positionChanged();
        }
    }
}

void Player::set_duration(int value) {
    if (value != std::exchange(m_duration, value)) Q_EMIT durationChanged();
}

void Player::set_playback_state(PlaybackState value) {
    if (auto old = std::exchange(m_playback_state, value); old != value) {
        Q_EMIT playbackStateChanged(old, value);
    }
}

void Player::set_cache_progress(QVector2D value) {
    do {
        if (m_source.isLocalFile()) {
            value = { 0.0, 1.0 };
            break;
        }
        if (! ycore::equal_within_ulps(m_cache_progress.x(), value.x(), 4)) break;
        if (ycore::equal_within_ulps(1.0f, value.y(), 4)) break;
        auto delta = value.y() - m_cache_progress.y();
        if (delta < 0 || delta > 0.05) break;
        return;
    } while (false);
    m_cache_progress = value;
    Q_EMIT cacheProgressChanged();
}

void Player::seek(double position) {
    set_position(position * duration());
    Q_EMIT seeked(this->position() * 1000.0);
}

void Player::processNotify(NotifyInfo info) {
    using namespace player;
    std::visit(overloaded {
                   [](notify::position) {},
                   [this](notify::duration value) {
                       set_duration(static_cast<int>(value.value));
                   },
                   [this](notify::playstate value) {
                       set_playback_state(static_cast<PlaybackState>(value.value));
                   },
                   [this](notify::busy value) {
                       set_busy(value.value);
                   },
                   [this](notify::cache value) {
                       set_cache_progress({ value.begin, value.end });
                   },
               },
               info);
}

#include "Qcm/player.moc.cpp"
