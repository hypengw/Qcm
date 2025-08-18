#include "mpris/mediaplayer2.h"
#include "mpris/mediaplayer2_adaptor.h"
#include "core/core.h"

#include <span>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QUrl>

using namespace mpris;
using namespace Qt::Literals::StringLiterals;

namespace
{
static const QString MprisObjectPath { u"/org/mpris/MediaPlayer2"_s };
static const QString MprisPlayerObjectPath { u"/org/mpris/MediaPlayer2.Player"_s };
static const QString DBusPropertiesInterface { u"org.freedesktop.DBus.Properties"_s };
static const QString DBusPropertiesChangedSignal { u"PropertiesChanged"_s };
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

QString MediaPlayer2::static_metakey(MetaKey k) {
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

QString MediaPlayer2::metakey(MetaKey k) const { return MediaPlayer2::static_metakey(k); };

// Mpris2 Root Interface
bool MediaPlayer2::canQuit() const { return m_canQuit; }

void MediaPlayer2::setCanQuit(bool v) {
    if (ycore::cmp_set(m_canQuit, v)) {
        emit canQuitChanged();
    }
}

bool MediaPlayer2::canRaise() const { return m_canRaise; }

void MediaPlayer2::setCanRaise(bool v) {
    if (ycore::cmp_set(m_canRaise, v)) {
        emit canRaiseChanged();
    }
}

bool MediaPlayer2::canSetFullscreen() const { return m_canSetFullscreen; }

void MediaPlayer2::setCanSetFullscreen(bool v) {
    if (ycore::cmp_set(m_canSetFullscreen, v)) {
        emit canSetFullscreenChanged();
    }
}

QString MediaPlayer2::desktopEntry() const { return m_desktopEntry; }

void MediaPlayer2::setDesktopEntry(const QString& v) {
    if (ycore::cmp_set(m_desktopEntry, v)) {
        emit desktopEntryChanged();
    }
}

bool MediaPlayer2::fullscreen() const { return m_fullscreen; }

void MediaPlayer2::setFullscreen(bool v) {
    if (ycore::cmp_set(m_fullscreen, v)) {
        emit fullscreenChanged();
    }
}

bool MediaPlayer2::hasTrackList() const { return m_hasTrackList; }

void MediaPlayer2::setHasTrackList(bool v) {
    if (ycore::cmp_set(m_hasTrackList, v)) {
        emit hasTrackListChanged();
    }
}

QString MediaPlayer2::identity() const { return m_identity; }

void MediaPlayer2::setIdentity(const QString& v) {
    if (ycore::cmp_set(m_identity, v)) {
        emit identityChanged();
    }
}

QStringList MediaPlayer2::supportedUriSchemes() const { return m_supportedUriSchemes; }

void MediaPlayer2::setSupportedUriSchemes(const QStringList& v) {
    if (ycore::cmp_set(m_supportedUriSchemes, v)) {
        emit supportedUriSchemesChanged();
    }
}

QStringList MediaPlayer2::supportedMimeTypes() const { return m_supportedMimeTypes; }

void MediaPlayer2::setSupportedMimeTypes(const QStringList& v) {
    if (ycore::cmp_set(m_supportedMimeTypes, v)) {
        emit supportedMimeTypesChanged();
    }
}

// Mpris2 Player Interface
bool MediaPlayer2::canControl() const { return m_canControl; }

void MediaPlayer2::setCanControl(bool v) {
    if (ycore::cmp_set(m_canControl, v)) {
        emit canControlChanged();
    }
}

bool MediaPlayer2::canGoNext() const { return m_canGoNext; }

void MediaPlayer2::setCanGoNext(bool v) {
    if (ycore::cmp_set(m_canGoNext, v)) {
        emit canGoNextChanged();
    }
}

bool MediaPlayer2::canGoPrevious() const { return m_canGoPrevious; }

void MediaPlayer2::setCanGoPrevious(bool v) {
    if (ycore::cmp_set(m_canGoPrevious, v)) {
        emit canGoPreviousChanged();
    }
}

bool MediaPlayer2::canPause() const { return m_canPause; }

void MediaPlayer2::setCanPause(bool v) {
    if (ycore::cmp_set(m_canPause, v)) {
        emit canPauseChanged();
    }
}

bool MediaPlayer2::canPlay() const { return m_canPlay; }

void MediaPlayer2::setCanPlay(bool v) {
    if (ycore::cmp_set(m_canPlay, v)) {
        emit canPlayChanged();
    }
}

bool MediaPlayer2::canSeek() const { return m_canSeek; }

void MediaPlayer2::setCanSeek(bool v) {
    if (ycore::cmp_set(m_canSeek, v)) {
        emit canSeekChanged();
    }
}

MediaPlayer2::Loop_Status MediaPlayer2::loopStatus() const { return m_loopStatus; }

void MediaPlayer2::setLoopStatus(Loop_Status v) {
    if (ycore::cmp_set(m_loopStatus, v)) {
        emit loopStatusChanged();
    }
}

double MediaPlayer2::maximumRate() const { return m_maximumRate; }

void MediaPlayer2::setMaximumRate(double v) {
    if (ycore::cmp_set(m_maximumRate, v)) {
        emit maximumRateChanged();
    }
}

QVariantMap MediaPlayer2::metadata() const { return m_typedMetadata; }

namespace
{

template<typename Fn>
void metadata_type_map(QVariantMap& meta, std::span<const MediaPlayer2::MetaKey> keys, Fn&& fn) {
    for (auto& k : keys) {
        auto k_s = MediaPlayer2::static_metakey(k);
        if (meta.contains(k_s)) meta[k_s] = fn(meta[k_s]);
    }
}

} // namespace

void MediaPlayer2::setMetadata(const QVariantMap& metadata) {
    if (m_metadata == metadata) {
        return;
    }

    m_metadata       = metadata;
    m_typedMetadata  = m_metadata;
    auto key_trackid = metakey(MetaTrackId);
    if (m_typedMetadata.contains(key_trackid)) {
        m_typedMetadata[key_trackid] =
            QVariant::fromValue(QDBusObjectPath(m_metadata[key_trackid].toString()));
    }
    metadata_type_map(m_typedMetadata,
                      std::array {
                          MetaAlbumArtist,
                          MetaArtist,
                          MetaComment,
                          MetaComposer,
                          MetaGenre,
                          MetaLyricist,
                      },
                      [](auto& v) {
                          return QVariant::fromValue(v).toStringList();
                      });
    Q_EMIT metadataChanged();
}

double MediaPlayer2::minimumRate() const { return m_minimumRate; }

void MediaPlayer2::setMinimumRate(double v) {
    if (ycore::cmp_set(m_minimumRate, v)) {
        emit minimumRateChanged();
    }
}

MediaPlayer2::Playback_Status MediaPlayer2::playbackStatus() const { return m_playbackStatus; }

void MediaPlayer2::setPlaybackStatus(Playback_Status v) {
    if (ycore::cmp_set(m_playbackStatus, v)) {
        emit playbackStatusChanged();
    }
}

qlonglong MediaPlayer2::position() const { return m_position; }

void MediaPlayer2::setPosition(qlonglong v) {
    if (ycore::cmp_set(m_position, v)) {
        emit positionChanged();
    }
}

double MediaPlayer2::rate() const { return m_rate; }

void MediaPlayer2::setRate(double v) {
    if (ycore::cmp_set(m_rate, v)) {
        emit rateChanged();
    }
}

bool MediaPlayer2::shuffle() const { return m_shuffle; }

void MediaPlayer2::setShuffle(bool v) {
    if (ycore::cmp_set(m_shuffle, v)) {
        emit shuffleChanged();
    }
}

double MediaPlayer2::volume() const { return m_volume; }

void MediaPlayer2::setVolume(double v) {
    if (ycore::cmp_set(m_volume, v)) {
        emit volumeChanged();
    }
}

// Private
/*
QVariantMap MediaPlayer2::typeMetadata(const QVariantMap& aMetadata) {
    QVariantMap                 metadata;
    QVariantMap::const_iterator i = aMetadata.constBegin();
    while (i != aMetadata.constEnd()) {
        switch (MetaenumerationFromString<MetaMetadata>(i.key())) {
        case MetaTrackId:
            metadata.insert(i.key(), QVariant::fromValue(QDBusObjectPath(i.value().toString())));
            break;
        case MetaLength:
            metadata.insert(i.key(), QVariant::fromValue(i.value().toLongLong()));
            break;
        case MetaArtUrl:
        case MetaUrl:
            metadata.insert(i.key(), QVariant::fromValue(i.value().toUrl().toString()));
            break;
        case MetaAlbum:
        case MetaAsText:
        case MetaTitle: metadata.insert(i.key(), QVariant::fromValue(i.value().toString())); break;
        case MetaAlbumArtist:
        case MetaArtist:
        case MetaComment:
        case MetaComposer:
        case MetaGenre:
        case MetaLyricist:
            metadata.insert(i.key(), QVariant::fromValue(i.value().toStringList()));
            break;
        case MetaAudioBPM:
        case MetaDiscNumber:
        case MetaTrackNumber:
        case MetaUseCount: metadata.insert(i.key(), QVariant::fromValue(i.value().toInt())); break;
        case MetaAutoRating:
        case MetaUserRating:
            metadata.insert(i.key(), QVariant::fromValue(i.value().toFloat()));
            break;
        case MetaContentCreated:
        case MetaFirstUsed:
        case MetaLastUsed:
            metadata.insert(i.key(), QVariant::fromValue(i.value().toDate().toString(Qt::ISODate)));
            break;
        case MetaInvalidMetadata:
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


#include <mpris/moc_mediaplayer2.cpp>