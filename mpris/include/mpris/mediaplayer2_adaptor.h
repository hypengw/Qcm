#pragma once

#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

namespace mpris
{

class MediaPlayer2;
class MediaPlayer2Adaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    // https://specifications.freedesktop.org/mpris-spec/latest/Media_Player.html

public:
    MediaPlayer2Adaptor(MediaPlayer2* parent);

public:
    Q_PROPERTY(bool CanQuit READ CanQuit)
    bool CanQuit() const;

    Q_PROPERTY(bool CanRaise READ CanRaise)
    bool CanRaise() const;

    Q_PROPERTY(bool CanSetFullscreen READ CanSetFullscreen)
    bool CanSetFullscreen() const;

    Q_PROPERTY(QString DesktopEntry READ DesktopEntry)
    QString DesktopEntry() const;

    Q_PROPERTY(bool Fullscreen READ Fullscreen WRITE setFullscreen)
    bool Fullscreen() const;
    void setFullscreen(bool value);

    Q_PROPERTY(bool HasTrackList READ HasTrackList)
    bool HasTrackList() const;

    Q_PROPERTY(QString Identity READ Identity)
    QString Identity() const;

    Q_PROPERTY(QStringList SupportedMimeTypes READ SupportedMimeTypes)
    QStringList SupportedMimeTypes() const;

    Q_PROPERTY(QStringList SupportedUriSchemes READ SupportedUriSchemes)
    QStringList SupportedUriSchemes() const;

public slots:
    // Methods
    void Quit();
    void Raise();

private slots:
    void onCanQuitChanged() const;
    void onCanRaiseChanged() const;
    void onCanSetFullscreenChanged() const;
    void onDesktopEntryChanged() const;
    void onFullscreenChanged() const;
    void onHasTrackListChanged() const;
    void onIdentityChanged() const;
    void onSupportedUriSchemesChanged() const;
    void onSupportedMimeTypesChanged() const;

private:
    MediaPlayer2* m_realobj;
};

class MediaPlayer2PlayerAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    // https://specifications.freedesktop.org/mpris-spec/latest/Player_Interface.html

    // Properties
    Q_PROPERTY(QString PlaybackStatus READ PlaybackStatus)
    Q_PROPERTY(QString LoopStatus READ LoopStatus WRITE setLoopStatus)
    Q_PROPERTY(double Rate READ Rate WRITE setRate)
    Q_PROPERTY(bool Shuffle READ Shuffle WRITE setShuffle)
    Q_PROPERTY(QVariantMap Metadata READ Metadata)
    Q_PROPERTY(double Volume READ Volume WRITE setVolume)
    Q_PROPERTY(qlonglong Position READ Position)
    Q_PROPERTY(double MinimumRate READ MinimumRate)
    Q_PROPERTY(double MaximumRate READ MaximumRate)
    Q_PROPERTY(bool CanGoNext READ CanGoNext)
    Q_PROPERTY(bool CanGoPrevious READ CanGoPrevious)
    Q_PROPERTY(bool CanPlay READ CanPlay)
    Q_PROPERTY(bool CanPause READ CanPause)
    Q_PROPERTY(bool CanSeek READ CanSeek)
    Q_PROPERTY(bool CanControl READ CanControl)

public:
    explicit MediaPlayer2PlayerAdaptor(MediaPlayer2* parent = nullptr);

    QString     PlaybackStatus() const;
    QString     LoopStatus() const;
    double      Rate() const;
    bool        Shuffle() const;
    QVariantMap Metadata() const;
    double      Volume() const;
    qlonglong   Position() const;
    double      MinimumRate() const;
    double      MaximumRate() const;
    bool        CanGoNext() const;
    bool        CanGoPrevious() const;
    bool        CanPlay() const;
    bool        CanPause() const;
    bool        CanSeek() const;
    bool        CanControl() const;

    void setLoopStatus(QString);
    void setRate(double);
    void setShuffle(bool);
    void setVolume(double);

signals:
    void Seeked(qlonglong Position);

public slots:
    // Methods
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong Offset);
    void SetPosition(const QDBusObjectPath& trackId, qlonglong pos);
    void OpenUri(const QString& uri);

private slots:
    void onCanControlChanged() const;
    void onCanGoNextChanged() const;
    void onCanGoPreviousChanged() const;
    void onCanPauseChanged() const;
    void onCanPlayChanged() const;
    void onCanSeekChanged() const;
    void onLoopStatusChanged() const;
    void onMaximumRateChanged() const;
    void onMetadataChanged() const;
    void onMinimumRateChanged() const;
    void onPlaybackStatusChanged() const;
    void onRateChanged() const;
    void onShuffleChanged() const;
    void onVolumeChanged() const;

private:
    MediaPlayer2* m_realobj;
};
} // namespace mpris
