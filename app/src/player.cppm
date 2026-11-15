module;
#include "QExtra/macro_qt.hpp"
#ifdef Q_MOC_RUN
#    include "Qcm/player.moc"
#endif

export module qcm:player;
export import :qml.enums;
import :backend;
import qextra;

namespace qcm
{
export class Player : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(QcmPlayer)
    QML_UNCREATABLE("QcmPlayer is owned by Global")

    Q_PROPERTY(
        QUrl source READ source WRITE set_source RESET reset_source NOTIFY sourceChanged FINAL)
    Q_PROPERTY(qint64 position READ position WRITE set_position NOTIFY positionChanged FINAL)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged FINAL)
    Q_PROPERTY(float volume READ volume WRITE set_volume NOTIFY volumeChanged FINAL)
    Q_PROPERTY(uint fadeTime READ fadeTime WRITE set_fadeTime NOTIFY fadeTimeChanged FINAL)
    Q_PROPERTY(qcm::enums::PlaybackState playbackState READ playback_state NOTIFY
                   playbackStateChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(bool changingTrack READ changingTrack NOTIFY changingTrackChanged FINAL)
    Q_PROPERTY(QVector2D cacheProgress READ cache_progress NOTIFY cacheProgressChanged FINAL)
    Q_PROPERTY(bool seekable READ seekable NOTIFY seekableChanged FINAL)
    Q_PROPERTY(bool playing READ playing NOTIFY playbackStateChanged FINAL)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged FINAL)

public:
    using PlaybackState = enums::PlaybackState;

    explicit Player(QObject* parent = nullptr);
    void bindBackend(Backend*);
    void applyState(const msg::PlayerStateMsg&);

    auto source() const -> QUrl;
    auto position() const -> qint64;
    auto duration() const -> qint64;
    auto busy() const -> bool;
    auto changingTrack() const -> bool;
    void setChangingTrack(bool);
    auto playback_state() const -> PlaybackState;
    auto volume() const -> float;
    auto fadeTime() const -> u32;
    auto cache_progress() const -> QVector2D;
    auto seekable() const -> bool;
    auto playing() const -> bool;
    auto connected() const -> bool;

    Q_SIGNAL void sourceChanged();
    Q_SIGNAL void positionChanged();
    Q_SIGNAL void durationChanged();
    Q_SIGNAL void volumeChanged(float);
    Q_SIGNAL void fadeTimeChanged(u32);
    Q_SIGNAL void busyChanged();
    Q_SIGNAL void changingTrackChanged();
    Q_SIGNAL void playbackStateChanged();
    Q_SIGNAL void cacheProgressChanged();
    Q_SIGNAL void seeked(double position);
    Q_SIGNAL void ended();
    Q_SIGNAL void seekableChanged();
    Q_SIGNAL void connectedChanged();
    Q_SIGNAL void errorOccurred(QString);

    Q_SLOT void set_source(const QUrl&);
    Q_SLOT void reset_source();
    Q_SLOT void set_position(qint64);
    Q_SLOT void set_volume(float);
    Q_SLOT void set_fadeTime(u32);
    Q_SLOT void seek(double position);
    Q_SLOT void toggle();
    Q_SLOT void play();
    Q_SLOT void pause();
    Q_SLOT void stop();

private:
    void command(msg::PlayerActionGadget::PlayerAction);
    void disconnectPlayer();

    QPointer<Backend>   m_backend;
    msg::PlayerStateMsg m_state;
    bool                m_has_snapshot {};
    bool                m_connected {};
    bool                m_changing_track {};
};
} // namespace qcm
