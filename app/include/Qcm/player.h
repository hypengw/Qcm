#pragma once

#include <QQmlEngine>
#include "player/player.h"
#include "player/notify.h"

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
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)

public:
    using NotifyInfo = player::notify::info;
    class NotifyChannel;


    enum PlaybackState
    {
        PlayingState = 0,
        PausedState,
        StoppedState,
    };
    Q_ENUM(PlaybackState)

    Player(QObject* = nullptr);
    ~Player();

    const QUrl& source() const;
    void        set_source(const QUrl&);
    auto        position() const -> int;
    auto        duration() const -> int;
    auto        busy() const -> bool;
    auto        playbackState() const -> PlaybackState;
    auto        volume() const -> float;
    auto        fadeTime() const -> u32;

    auto sender() const -> Sender<NotifyInfo>;

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();

private:
    void set_playback_state(PlaybackState);
    void set_duration(int);
    void set_position_raw(int);

Q_SIGNALS:
    void sourceChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void fadeTimeChanged();
    void busyChanged();
    void playbackStateChanged();
    void notify(NotifyInfo);

public Q_SLOTS:
    void processNotify(NotifyInfo);
    void set_position(int);
    void set_busy(bool);
    void set_volume(float);
    void set_fadeTime(u32);

private:
    up<player::Player> m_player;
    QUrl               m_source;
    rc<NotifyChannel>  m_channel;
    bool               m_end;

    std::atomic<int> m_position;
    int              m_duration;
    bool             m_busy;
    PlaybackState    m_playback_state;
};

} // namespace qcm
