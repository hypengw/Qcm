#pragma once

#include <chrono>
#include <QVector2D>
#include <QQmlEngine>
#include "player/player.h"
#include "player/notify.h"
#include "Qcm/qml/enum.hpp"

namespace qcm
{

class Player : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(QcmPlayer)

    Q_PROPERTY(QUrl source READ source WRITE set_source NOTIFY sourceChanged FINAL)
    Q_PROPERTY(int position READ position WRITE set_position NOTIFY positionChanged FINAL)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged FINAL)
    Q_PROPERTY(float volume READ volume WRITE set_volume NOTIFY volumeChanged FINAL)
    Q_PROPERTY(uint fadeTime READ fadeTime WRITE set_fadeTime NOTIFY fadeTimeChanged FINAL)
    Q_PROPERTY(qcm::enums::PlaybackState playbackState READ playback_state NOTIFY
                   playbackStateChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(QVector2D cacheProgress READ cache_progress NOTIFY cacheProgressChanged FINAL)

    Q_PROPERTY(bool seekable READ seekable CONSTANT FINAL)
    Q_PROPERTY(bool playing READ playing NOTIFY playbackStateChanged FINAL)

public:
    using NotifyInfo = player::notify::info;
    class NotifyChannel;
    using PlaybackState = enums::PlaybackState;

    Player(QObject* = nullptr);
    ~Player();

    const QUrl& source() const;
    auto        position() const -> int;
    auto        duration() const -> int;
    auto        busy() const -> bool;
    auto        playback_state() const -> PlaybackState;
    auto        volume() const -> float;
    auto        fadeTime() const -> u32;
    auto        cache_progress() const -> QVector2D;

    auto seekable() const -> bool;
    auto playing() const -> bool;

    auto sender() const -> Sender<NotifyInfo>;

    Q_SIGNAL void sourceChanged();
    Q_SIGNAL void positionChanged();
    Q_SIGNAL void durationChanged();
    Q_SIGNAL void volumeChanged();
    Q_SIGNAL void fadeTimeChanged();
    Q_SIGNAL void busyChanged();
    Q_SIGNAL void playbackStateChanged(PlaybackState old, PlaybackState new_);
    Q_SIGNAL void cacheProgressChanged();
    Q_SIGNAL void seeked(double position);
    Q_SIGNAL void notify(NotifyInfo);

    Q_SLOT void processNotify(NotifyInfo);
    Q_SLOT void set_source(const QUrl&);
    Q_SLOT void set_position(int);
    Q_SLOT void set_busy(bool);
    Q_SLOT void set_volume(float);
    Q_SLOT void set_fadeTime(u32);
    Q_SLOT void seek(double pos);

    Q_SLOT void toggle();
    Q_SLOT void play();
    Q_SLOT void pause();
    Q_SLOT void stop();

private:
    void set_playback_state(PlaybackState);
    void set_duration(int);
    void set_position_raw(int);
    void set_cache_progress(QVector2D);

private:
    up<player::Player> m_player;
    QUrl               m_source;
    rc<NotifyChannel>  m_channel;
    bool               m_end;

    std::atomic<std::chrono::steady_clock::time_point> m_last_time;

    std::atomic<int> m_position;
    int              m_duration;
    bool             m_busy;
    PlaybackState    m_playback_state;
    QVector2D        m_cache_progress;
};

} // namespace qcm
