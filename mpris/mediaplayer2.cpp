#include "mpris/mediaplayer2.h"
#include "mpris/mediaplayer2_adaptor.h"

#include <QDBusConnection>
#include <QDBusMessage>

using namespace mpris;

namespace
{
static const QString MprisObjectPath { u"/org/mpris/MediaPlayer2"_qs };
static const QString DBusPropertiesInterface { u"org.freedesktop.DBus.Properties"_qs };
static const QString DBusPropertiesChangedSignal { u"PropertiesChanged"_qs };
} // namespace

MediaPlayer2::MediaPlayer2(QObject* parent)
    : QObject(parent),
      m_adaptor_mp2(new MediaPlayer2Adaptor(this)),
      m_adaptor_mp2p(new MediaPlayer2PlayerAdaptor(this)),
      m_serviceName("player"),
      m_canQuit(false),
      m_canRaise(false),
      m_canSetFullscreen(false),
      m_fullscreen(false),
      m_hasTrackList(false),
      m_canControl(false),
      m_canGoNext(false),
      m_canGoPrevious(false),
      m_canPause(false),
      m_canPlay(false),
      m_canSeek(false),
      m_loopStatus(None),
      m_maximumRate(1),
      m_minimumRate(1),
      m_playbackStatus(Stopped),
      m_position(0),
      m_rate(1),
      m_shuffle(false),
      m_volume(0) {
    auto connection = QDBusConnection::sessionBus();
    if (connection.isConnected()) {
        if (! connection.registerObject(MprisObjectPath, this)) {
            qWarning() << "Mpris: Failed attempting to register object path. Already registered?";
        }
    }
}

MediaPlayer2::~MediaPlayer2() {}

QString MediaPlayer2::metakey(MetaKey k) const {
    constexpr std::array keys { "mpris:trackid",     "mpris:length",      "mpris:artUrl",
                                "xesam:album",       "xesam:albumArtist", "xesam:artist",
                                "xesam:asText",      "xesam:audioBPM",    "xesam:autoRating",
                                "xesam:comment",     "xesam:composer",    "xesam:contentCreated",
                                "xesam:discNumber",  "xesam:firstUsed",   "xesam:genre",
                                "xesam:lastUsed",    "xesam:lyricist",    "xesam:title",
                                "xesam:trackNumber", "xesam:url",         "xesam:useCount",
                                "xesam:userRating" };
    auto                 idx = (std::size_t)k;
    if (idx < keys.size()) {
        return keys.at(idx);
    }
    return "";
};

// Mpris2 Root Interface
bool MediaPlayer2::canQuit() const { return m_canQuit; }

void MediaPlayer2::setCanQuit(bool v) {
    if (std::exchange(m_canQuit, v) != v) {
        emit canQuitChanged();
    }
}

bool MediaPlayer2::canRaise() const { return m_canRaise; }

void MediaPlayer2::setCanRaise(bool v) {
    if (std::exchange(m_canRaise, v) != v) {
        emit canRaiseChanged();
    }
}

bool MediaPlayer2::canSetFullscreen() const { return m_canSetFullscreen; }

void MediaPlayer2::setCanSetFullscreen(bool v) {
    if (std::exchange(m_canSetFullscreen, v) != v) {
        emit canSetFullscreenChanged();
    }
}

QString MediaPlayer2::desktopEntry() const { return m_desktopEntry; }

void MediaPlayer2::setDesktopEntry(const QString& desktopEntry) {
    if (m_desktopEntry == desktopEntry) {
        return;
    }

    m_desktopEntry = desktopEntry;
    Q_EMIT desktopEntryChanged();
}

bool MediaPlayer2::fullscreen() const { return m_fullscreen; }

void MediaPlayer2::setFullscreen(bool v) {
    if (std::exchange(m_fullscreen, v) != v) {
        emit fullscreenChanged();
    }
}

bool MediaPlayer2::hasTrackList() const { return m_hasTrackList; }

void MediaPlayer2::setHasTrackList(bool hasTrackList) {
    if (m_hasTrackList == hasTrackList) {
        return;
    }

    m_hasTrackList = hasTrackList;
    Q_EMIT hasTrackListChanged();
}

QString MediaPlayer2::identity() const { return m_identity; }

void MediaPlayer2::setIdentity(const QString& identity) {
    if (m_identity == identity) {
        return;
    }

    m_identity = identity;
    Q_EMIT identityChanged();
}

QStringList MediaPlayer2::supportedUriSchemes() const { return m_supportedUriSchemes; }

void MediaPlayer2::setSupportedUriSchemes(const QStringList& supportedUriSchemes) {
    if (m_supportedUriSchemes == supportedUriSchemes) {
        return;
    }

    m_supportedUriSchemes = supportedUriSchemes;
    Q_EMIT supportedUriSchemesChanged();
}

QStringList MediaPlayer2::supportedMimeTypes() const { return m_supportedMimeTypes; }

void MediaPlayer2::setSupportedMimeTypes(const QStringList& supportedMimeTypes) {
    if (m_supportedMimeTypes == supportedMimeTypes) {
        return;
    }

    m_supportedMimeTypes = supportedMimeTypes;
    Q_EMIT supportedMimeTypesChanged();
}

// Mpris2 Player Interface
bool MediaPlayer2::canControl() const { return m_canControl; }

void MediaPlayer2::setCanControl(bool canControl) {
    if (m_canControl == canControl) {
        return;
    }

    m_canControl = canControl;
    Q_EMIT canControlChanged();
}

bool MediaPlayer2::canGoNext() const { return m_canGoNext; }

void MediaPlayer2::setCanGoNext(bool v) {
    if (std::exchange(m_canGoNext, v) != v) {
        emit canGoNextChanged();
    }
}

bool MediaPlayer2::canGoPrevious() const { return m_canGoPrevious; }

void MediaPlayer2::setCanGoPrevious(bool v) {
    if (std::exchange(m_canGoPrevious, v) != v) {
        emit canGoPreviousChanged();
    }
}

bool MediaPlayer2::canPause() const { return m_canPause; }

void MediaPlayer2::setCanPause(bool canPause) {
    if (m_canPause == canPause) {
        return;
    }

    m_canPause = canPause;
    Q_EMIT canPauseChanged();
}

bool MediaPlayer2::canPlay() const { return m_canPlay; }

void MediaPlayer2::setCanPlay(bool canPlay) {
    if (m_canPlay == canPlay) {
        return;
    }

    m_canPlay = canPlay;
    Q_EMIT canPlayChanged();
}

bool MediaPlayer2::canSeek() const { return m_canSeek; }

void MediaPlayer2::setCanSeek(bool canSeek) {
    if (m_canSeek == canSeek) {
        return;
    }

    m_canSeek = canSeek;
    Q_EMIT canSeekChanged();
}

MediaPlayer2::Loop_Status MediaPlayer2::loopStatus() const { return m_loopStatus; }

void MediaPlayer2::setLoopStatus(Loop_Status v) {
    if (std::exchange(m_loopStatus, v) != v) {
        emit loopStatusChanged();
    }
}

double MediaPlayer2::maximumRate() const { return m_maximumRate; }

void MediaPlayer2::setMaximumRate(double maximumRate) {
    if (m_maximumRate == maximumRate) {
        return;
    }

    m_maximumRate = maximumRate;
    Q_EMIT maximumRateChanged();
}

QVariantMap MediaPlayer2::metadata() const { return m_typedMetadata; }

void MediaPlayer2::setMetadata(const QVariantMap& metadata) {
    if (m_metadata == metadata) {
        return;
    }

    m_metadata       = metadata;
    m_typedMetadata  = m_metadata;
    auto key_trackid = metakey(MetaTrackId);
    if (m_typedMetadata.contains(key_trackid)) {
        m_typedMetadata[key_trackid] = QVariant::fromValue(
            QDBusObjectPath(u"/trackid/"_qs + m_metadata[key_trackid].toString()));
    }
    Q_EMIT metadataChanged();
}

double MediaPlayer2::minimumRate() const { return m_minimumRate; }

void MediaPlayer2::setMinimumRate(double minimumRate) {
    if (m_minimumRate == minimumRate) {
        return;
    }

    m_minimumRate = minimumRate;
    Q_EMIT minimumRateChanged();
}

MediaPlayer2::Playback_Status MediaPlayer2::playbackStatus() const { return m_playbackStatus; }

void MediaPlayer2::setPlaybackStatus(Playback_Status v) {
    if (std::exchange(m_playbackStatus, v) != v) {
        emit playbackStatusChanged();
    }
}

qlonglong MediaPlayer2::position() const { return m_position; }

void MediaPlayer2::setPosition(qlonglong position) {
    if (m_position == position) {
        return;
    }

    m_position = position;
    Q_EMIT positionChanged();
}

double MediaPlayer2::rate() const { return m_rate; }

void MediaPlayer2::setRate(double rate) {
    if (m_rate == rate) {
        return;
    }

    m_rate = rate;
    Q_EMIT rateChanged();
}

bool MediaPlayer2::shuffle() const { return m_shuffle; }

void MediaPlayer2::setShuffle(bool shuffle) {
    if (m_shuffle == shuffle) {
        return;
    }

    m_shuffle = shuffle;
    Q_EMIT shuffleChanged();
}

double MediaPlayer2::volume() const { return m_volume; }

void MediaPlayer2::setVolume(double volume) {
    if (m_volume == volume) {
        return;
    }

    m_volume = volume;
    Q_EMIT volumeChanged();
}

// Private

/*
QVariantMap MediaPlayer2::typeMetadata(const QVariantMap& aMetadata) {
    QVariantMap                 metadata;
    QVariantMap::const_iterator i = aMetadata.constBegin();
    while (i != aMetadata.constEnd()) {
        switch (Mpris::enumerationFromString<Mpris::Metadata>(i.key())) {
        case Mpris::TrackId:
            metadata.insert(i.key(),
QVariant::fromValue(QDBusObjectPath(i.value().toString()))); break; case Mpris::Length:
            metadata.insert(i.key(), QVariant::fromValue(i.value().toLongLong()));
            break;
        case Mpris::ArtUrl:
        case Mpris::Url:
            metadata.insert(i.key(), QVariant::fromValue(i.value().toUrl().toString()));
            break;
        case Mpris::Album:
        case Mpris::AsText:
        case Mpris::Title:
            metadata.insert(i.key(), QVariant::fromValue(i.value().toString()));
            break;
        case Mpris::AlbumArtist:
        case Mpris::Artist:
        case Mpris::Comment:
        case Mpris::Composer:
        case Mpris::Genre:
        case Mpris::Lyricist:
            metadata.insert(i.key(), QVariant::fromValue(i.value().toStringList()));
            break;
        case Mpris::AudioBPM:
        case Mpris::DiscNumber:
        case Mpris::TrackNumber:
        case Mpris::UseCount:
            metadata.insert(i.key(), QVariant::fromValue(i.value().toInt()));
            break;
        case Mpris::AutoRating:
        case Mpris::UserRating:
            metadata.insert(i.key(), QVariant::fromValue(i.value().toFloat()));
            break;
        case Mpris::ContentCreated:
        case Mpris::FirstUsed:
        case Mpris::LastUsed:
            metadata.insert(i.key(),
QVariant::fromValue(i.value().toDate().toString(Qt::ISODate))); break; case
Mpris::InvalidMetadata:
            // Passing with the original type and hoping the user used
            // a type supported by DBus
            metadata.insert(i.key(), i.value());
            break;
        default:
            // Nothing to do
            break;
        }

        ++i;
    }

    return metadata;
}
*/

void MediaPlayer2::notifyPropertiesChanged(const QString&     interfaceName,
                                           const QVariantMap& changedProperties,
                                           const QStringList& invalidatedProperties) const {
    QDBusMessage msg = QDBusMessage::createSignal(
        MprisObjectPath, DBusPropertiesInterface, DBusPropertiesChangedSignal);

    msg << interfaceName;
    msg << changedProperties;
    msg << invalidatedProperties;

    if (! QDBusConnection::sessionBus().send(msg)) {
    }
}
