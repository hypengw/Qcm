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
    Q_PROPERTY(bool CanQuit READ CanQuit FINAL)
    bool CanQuit() const;

    Q_PROPERTY(bool CanRaise READ CanRaise FINAL)
    bool CanRaise() const;

    Q_PROPERTY(bool CanSetFullscreen READ CanSetFullscreen FINAL)
    bool CanSetFullscreen() const;

    Q_PROPERTY(QString DesktopEntry READ DesktopEntry FINAL)
    QString DesktopEntry() const;

    Q_PROPERTY(bool Fullscreen READ Fullscreen WRITE setFullscreen FINAL)
    bool Fullscreen() const;
    void setFullscreen(bool value);

    Q_PROPERTY(bool HasTrackList READ HasTrackList FINAL)
    bool HasTrackList() const;

    Q_PROPERTY(QString Identity READ Identity FINAL)
    QString Identity() const;

    Q_PROPERTY(QStringList SupportedMimeTypes READ SupportedMimeTypes FINAL)
    QStringList SupportedMimeTypes() const;

    Q_PROPERTY(QStringList SupportedUriSchemes READ SupportedUriSchemes FINAL)
    QStringList SupportedUriSchemes() const;

    // Methods
    Q_SLOT void Quit();
    Q_SLOT void Raise();

private:
    Q_SLOT void onCanQuitChanged() const;
    Q_SLOT void onCanRaiseChanged() const;
    Q_SLOT void onCanSetFullscreenChanged() const;
    Q_SLOT void onDesktopEntryChanged() const;
    Q_SLOT void onFullscreenChanged() const;
    Q_SLOT void onHasTrackListChanged() const;
    Q_SLOT void onIdentityChanged() const;
    Q_SLOT void onSupportedUriSchemesChanged() const;
    Q_SLOT void onSupportedMimeTypesChanged() const;

private:
    MediaPlayer2* m_realobj;
};

class MediaPlayer2PlayerAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    // https://specifications.freedesktop.org/mpris-spec/latest/Player_Interface.html

    // Properties
    Q_PROPERTY(QString PlaybackStatus READ PlaybackStatus FINAL)
    Q_PROPERTY(QString LoopStatus READ LoopStatus WRITE setLoopStatus FINAL)
    Q_PROPERTY(double Rate READ Rate WRITE setRate FINAL)
    Q_PROPERTY(bool Shuffle READ Shuffle WRITE setShuffle FINAL)
    Q_PROPERTY(QVariantMap Metadata READ Metadata FINAL)
    Q_PROPERTY(double Volume READ Volume WRITE setVolume FINAL)
    Q_PROPERTY(qlonglong Position READ Position FINAL)
    Q_PROPERTY(double MinimumRate READ MinimumRate FINAL)
    Q_PROPERTY(double MaximumRate READ MaximumRate FINAL)
    Q_PROPERTY(bool CanGoNext READ CanGoNext FINAL)
    Q_PROPERTY(bool CanGoPrevious READ CanGoPrevious FINAL)
    Q_PROPERTY(bool CanPlay READ CanPlay FINAL)
    Q_PROPERTY(bool CanPause READ CanPause FINAL)
    Q_PROPERTY(bool CanSeek READ CanSeek FINAL)
    Q_PROPERTY(bool CanControl READ CanControl FINAL)

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

    Q_SIGNAL void Seeked(qlonglong Position);

    // Methods
    Q_SLOT void Next();
    Q_SLOT void Previous();
    Q_SLOT void Pause();
    Q_SLOT void PlayPause();
    Q_SLOT void Stop();
    Q_SLOT void Play();
    Q_SLOT void Seek(qlonglong Offset);
    Q_SLOT void SetPosition(const QDBusObjectPath& trackId, qlonglong pos);
    Q_SLOT void OpenUri(const QString& uri);

private:
    Q_SLOT void onCanControlChanged() const;
    Q_SLOT void onCanGoNextChanged() const;
    Q_SLOT void onCanGoPreviousChanged() const;
    Q_SLOT void onCanPauseChanged() const;
    Q_SLOT void onCanPlayChanged() const;
    Q_SLOT void onCanSeekChanged() const;
    Q_SLOT void onLoopStatusChanged() const;
    Q_SLOT void onMaximumRateChanged() const;
    Q_SLOT void onMetadataChanged() const;
    Q_SLOT void onMinimumRateChanged() const;
    Q_SLOT void onPlaybackStatusChanged() const;
    Q_SLOT void onRateChanged() const;
    Q_SLOT void onShuffleChanged() const;
    Q_SLOT void onVolumeChanged() const;

private:
    MediaPlayer2* m_realobj;
};
} // namespace mpris
