#pragma once

#include <QVector2D>
#include <QQmlEngine>
#include "player/player.h"
#include "player/notify.h"
#include "qcm_interface/enum.h"

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
    Q_PROPERTY(PlaybackState playbackState READ playback_state NOTIFY playbackStateChanged FINAL)
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

private:
    void set_playback_state(PlaybackState);
    void set_duration(int);
    void set_position_raw(int);
    void set_cache_progress(QVector2D);

Q_SIGNALS:
    void sourceChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void fadeTimeChanged();
    void busyChanged();
    void playbackStateChanged();
    void cacheProgressChanged();
    void seeked(double position);
    void notify(NotifyInfo);

public Q_SLOTS:
    void processNotify(NotifyInfo);
    void set_source(const QUrl&);
    void set_position(int);
    void set_busy(bool);
    void set_volume(float);
    void set_fadeTime(u32);
    void seek(double pos);

    void play();
    void pause();
    void stop();

private:
    up<player::Player> m_player;
    QUrl               m_source;
    rc<NotifyChannel>  m_channel;
    bool               m_end;

    std::atomic<int> m_position;
    int              m_duration;
    bool             m_busy;
    PlaybackState    m_playback_state;
    QVector2D        m_cache_progress;
};

} // namespace qcm
