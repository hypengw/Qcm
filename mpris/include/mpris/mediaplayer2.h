#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QStringList>

namespace mpris
{
class MediaPlayer2Adaptor;
class MediaPlayer2PlayerAdaptor;
class MediaPlayer2 : public QObject {
    Q_OBJECT

    // Root
    Q_PROPERTY(bool canQuit READ canQuit WRITE setCanQuit NOTIFY canQuitChanged FINAL)
    Q_PROPERTY(bool canRaise READ canRaise WRITE setCanRaise NOTIFY canRaiseChanged FINAL)
    Q_PROPERTY(bool canSetFullscreen READ canSetFullscreen WRITE setCanSetFullscreen NOTIFY
                   canSetFullscreenChanged FINAL)

    Q_PROPERTY(QString desktopEntry READ desktopEntry WRITE setDesktopEntry NOTIFY
                   desktopEntryChanged FINAL)
    Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged FINAL)
    Q_PROPERTY(
        bool hasTrackList READ hasTrackList WRITE setHasTrackList NOTIFY hasTrackListChanged FINAL)
    Q_PROPERTY(QString identity READ identity WRITE setIdentity NOTIFY identityChanged FINAL)
    Q_PROPERTY(QStringList supportedUriSchemes READ supportedUriSchemes WRITE setSupportedUriSchemes
                   NOTIFY supportedUriSchemesChanged FINAL)
    Q_PROPERTY(QStringList supportedMimeTypes READ supportedMimeTypes WRITE setSupportedMimeTypes
                   NOTIFY supportedMimeTypesChanged FINAL)

    // Player
    Q_PROPERTY(bool canControl READ canControl WRITE setCanControl NOTIFY canControlChanged FINAL)
    Q_PROPERTY(bool canGoNext READ canGoNext WRITE setCanGoNext NOTIFY canGoNextChanged FINAL)
    Q_PROPERTY(bool canGoPrevious READ canGoPrevious WRITE setCanGoPrevious NOTIFY
                   canGoPreviousChanged FINAL)
    Q_PROPERTY(bool canPause READ canPause WRITE setCanPause NOTIFY canPauseChanged FINAL)
    Q_PROPERTY(bool canPlay READ canPlay WRITE setCanPlay NOTIFY canPlayChanged FINAL)
    Q_PROPERTY(bool canSeek READ canSeek WRITE setCanSeek NOTIFY canSeekChanged FINAL)
    Q_PROPERTY(
        Loop_Status loopStatus READ loopStatus WRITE setLoopStatus NOTIFY loopStatusChanged FINAL)
    Q_PROPERTY(
        double maximumRate READ maximumRate WRITE setMaximumRate NOTIFY maximumRateChanged FINAL)
    Q_PROPERTY(QVariantMap metadata READ metadata WRITE setMetadata NOTIFY metadataChanged FINAL)
    Q_PROPERTY(
        double minimumRate READ minimumRate WRITE setMinimumRate NOTIFY minimumRateChanged FINAL)
    Q_PROPERTY(Playback_Status playbackStatus READ playbackStatus WRITE setPlaybackStatus NOTIFY
                   playbackStatusChanged FINAL)
    Q_PROPERTY(qlonglong position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(double rate READ rate WRITE setRate NOTIFY rateChanged FINAL)
    Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged FINAL)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged FINAL)

public:
    enum Playback_Status
    {
        Playing = 0,
        Paused,
        Stopped
    };
    Q_ENUM(Playback_Status);

    enum Loop_Status
    {
        None = 0,
        Track,
        Playlist
    };
    Q_ENUM(Loop_Status);

    enum MetaKey
    {
        MetaTrackId = 0,
        MetaLength,
        MetaArtUrl,
        MetaAlbum,
        MetaAlbumArtist,
        MetaArtist,
        MetaAsText,
        MetaAudioBPM,
        MetaAutoRating,
        MetaComment,
        MetaComposer,
        MetaContentCreated,
        MetaDiscNumber,
        MetaFirstUsed,
        MetaGenre,
        MetaLastUsed,
        MetaLyricist,
        MetaTitle,
        MetaTrackNumber,
        MetaUrl,
        MetaUseCount,
        MetaUserRating
    };
    Q_ENUM(MetaKey);

    explicit MediaPlayer2(QObject* parent = nullptr);
    virtual ~MediaPlayer2() override;

    static QString      static_metakey(MetaKey);
    Q_INVOKABLE QString metakey(MetaKey) const;
    // Mpris2 Root Interface
    bool canQuit() const;
    void setCanQuit(bool canQuit);

    bool canRaise() const;
    void setCanRaise(bool canRaise);

    bool canSetFullscreen() const;
    void setCanSetFullscreen(bool canSetFullscreen);

    QString desktopEntry() const;
    void    setDesktopEntry(const QString& desktopEntry);

    bool fullscreen() const;
    void setFullscreen(bool fullscreen);

    bool hasTrackList() const;
    void setHasTrackList(bool hasTrackList);

    QString identity() const;
    void    setIdentity(const QString& identity);

    QStringList supportedUriSchemes() const;
    void        setSupportedUriSchemes(const QStringList& supportedUriSchemes);

    QStringList supportedMimeTypes() const;
    void        setSupportedMimeTypes(const QStringList& supportedMimeTypes);

    // Mpris2 Player Interface
    bool canControl() const;
    void setCanControl(bool canControl);

    bool canGoNext() const;
    void setCanGoNext(bool canGoNext);

    bool canGoPrevious() const;
    void setCanGoPrevious(bool canGoPrevious);

    bool canPause() const;
    void setCanPause(bool canPause);

    bool canPlay() const;
    void setCanPlay(bool canPlay);

    bool canSeek() const;
    void setCanSeek(bool canSeek);

    Loop_Status loopStatus() const;
    void        setLoopStatus(Loop_Status loopStatus);

    double maximumRate() const;
    void   setMaximumRate(double maximumRate);

    QVariantMap metadata() const;
    void        setMetadata(const QVariantMap& metadata);

    double minimumRate() const;
    void   setMinimumRate(double minimumRate);

    Playback_Status playbackStatus() const;
    void            setPlaybackStatus(Playback_Status playbackStatus);

    qlonglong position() const;
    void      setPosition(qlonglong position);

    double rate() const;
    void   setRate(double rate);

    bool shuffle() const;
    void setShuffle(bool shuffle);

    double volume() const;
    void   setVolume(double volume);

    Q_SIGNAL void serviceNameChanged();

    // Mpris2 Root Interface
    Q_SIGNAL void canQuitChanged();
    Q_SIGNAL void canRaiseChanged();
    Q_SIGNAL void canSetFullscreenChanged();
    Q_SIGNAL void desktopEntryChanged();
    Q_SIGNAL void fullscreenChanged();
    Q_SIGNAL void hasTrackListChanged();
    Q_SIGNAL void identityChanged();
    Q_SIGNAL void supportedUriSchemesChanged();
    Q_SIGNAL void supportedMimeTypesChanged();
    Q_SIGNAL void fullscreenRequested(bool fullscreen);
    Q_SIGNAL void quitRequested();
    Q_SIGNAL void raiseRequested();

    // Mpris2 Player Interface
    Q_SIGNAL void canControlChanged();
    Q_SIGNAL void canGoNextChanged();
    Q_SIGNAL void canGoPreviousChanged();
    Q_SIGNAL void canPauseChanged();
    Q_SIGNAL void canPlayChanged();
    Q_SIGNAL void canSeekChanged();
    Q_SIGNAL void loopStatusChanged();
    Q_SIGNAL void maximumRateChanged();
    Q_SIGNAL void metadataChanged();
    Q_SIGNAL void minimumRateChanged();
    Q_SIGNAL void playbackStatusChanged();
    Q_SIGNAL void positionChanged();
    Q_SIGNAL void rateChanged();
    Q_SIGNAL void shuffleChanged();
    Q_SIGNAL void volumeChanged();

    // request
    Q_SIGNAL void loopStatusRequested(Loop_Status loopStatus);
    Q_SIGNAL void rateRequested(double rate);
    Q_SIGNAL void shuffleRequested(bool shuffle);
    Q_SIGNAL void volumeRequested(double volume);
    Q_SIGNAL void nextRequested();
    Q_SIGNAL void openUriRequested(const QUrl& url);
    Q_SIGNAL void pauseRequested();
    Q_SIGNAL void playRequested();
    Q_SIGNAL void playPauseRequested();
    Q_SIGNAL void previousRequested();
    Q_SIGNAL void seekRequested(qlonglong offset);
    Q_SIGNAL void seeked(qlonglong position);
    Q_SIGNAL void setPositionRequested(const QDBusObjectPath& trackId, qlonglong position);
    Q_SIGNAL void stopRequested();

private:
    void notifyPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties,
                                 const QStringList& invalidatedProperties) const;

    MediaPlayer2Adaptor*       m_adaptor_mp2;
    MediaPlayer2PlayerAdaptor* m_adaptor_mp2p;

    QString m_serviceName;

    // Mpris2 Root Interface
    bool        m_canQuit;
    bool        m_canRaise;
    bool        m_canSetFullscreen;
    QString     m_desktopEntry;
    bool        m_fullscreen;
    bool        m_hasTrackList;
    QString     m_identity;
    QStringList m_supportedUriSchemes;
    QStringList m_supportedMimeTypes;

    // Mpris2 Player Interface
    bool            m_canControl;
    bool            m_canGoNext;
    bool            m_canGoPrevious;
    bool            m_canPause;
    bool            m_canPlay;
    bool            m_canSeek;
    Loop_Status     m_loopStatus;
    double          m_maximumRate;
    QVariantMap     m_metadata;
    QVariantMap     m_typedMetadata;
    double          m_minimumRate;
    Playback_Status m_playbackStatus;
    qlonglong       m_position;
    double          m_rate;
    bool            m_shuffle;
    double          m_volume;

    friend class MediaPlayer2Adaptor;
    friend class MediaPlayer2PlayerAdaptor;
};
} // namespace mpris
