#include "mpris/mediaplayer2.h"
#include "mpris/mediaplayer2_adaptor.h"

#include <span>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QUrl>

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

void MediaPlayer2::setDesktopEntry(const QString& v) {
    if (std::exchange(m_desktopEntry, v) != v) {
        emit desktopEntryChanged();
    }
}

bool MediaPlayer2::fullscreen() const { return m_fullscreen; }

void MediaPlayer2::setFullscreen(bool v) {
    if (std::exchange(m_fullscreen, v) != v) {
        emit fullscreenChanged();
    }
}

bool MediaPlayer2::hasTrackList() const { return m_hasTrackList; }

void MediaPlayer2::setHasTrackList(bool v) {
    if (std::exchange(m_hasTrackList, v) != v) {
        emit hasTrackListChanged();
    }
}

QString MediaPlayer2::identity() const { return m_identity; }

void MediaPlayer2::setIdentity(const QString& v) {
    if (std::exchange(m_identity, v) != v) {
        emit identityChanged();
    }
}

QStringList MediaPlayer2::supportedUriSchemes() const { return m_supportedUriSchemes; }

void MediaPlayer2::setSupportedUriSchemes(const QStringList& v) {
    if (std::exchange(m_supportedUriSchemes, v) != v) {
        emit supportedUriSchemesChanged();
    }
}

QStringList MediaPlayer2::supportedMimeTypes() const { return m_supportedMimeTypes; }

void MediaPlayer2::setSupportedMimeTypes(const QStringList& v) {
    if (std::exchange(m_supportedMimeTypes, v) != v) {
        emit supportedMimeTypesChanged();
    }
}

// Mpris2 Player Interface
bool MediaPlayer2::canControl() const { return m_canControl; }

void MediaPlayer2::setCanControl(bool v) {
    if (std::exchange(m_canControl, v) != v) {
        emit canControlChanged();
    }
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

void MediaPlayer2::setCanPause(bool v) {
    if (std::exchange(m_canPause, v) != v) {
        emit canPauseChanged();
    }
}

bool MediaPlayer2::canPlay() const { return m_canPlay; }

void MediaPlayer2::setCanPlay(bool v) {
    if (std::exchange(m_canPlay, v) != v) {
        emit canPlayChanged();
    }
}

bool MediaPlayer2::canSeek() const { return m_canSeek; }

void MediaPlayer2::setCanSeek(bool v) {
    if (std::exchange(m_canSeek, v) != v) {
        emit canSeekChanged();
    }
}

MediaPlayer2::Loop_Status MediaPlayer2::loopStatus() const { return m_loopStatus; }

void MediaPlayer2::setLoopStatus(Loop_Status v) {
    if (std::exchange(m_loopStatus, v) != v) {
        emit loopStatusChanged();
    }
}

double MediaPlayer2::maximumRate() const { return m_maximumRate; }

void MediaPlayer2::setMaximumRate(double v) {
    if (std::exchange(m_maximumRate, v) != v) {
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
        m_typedMetadata[key_trackid] = QVariant::fromValue(
            QDBusObjectPath(u"/trackid/"_qs + m_metadata[key_trackid].toString()));
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
    if (std::exchange(m_minimumRate, v) != v) {
        emit minimumRateChanged();
    }
}

MediaPlayer2::Playback_Status MediaPlayer2::playbackStatus() const { return m_playbackStatus; }

void MediaPlayer2::setPlaybackStatus(Playback_Status v) {
    if (std::exchange(m_playbackStatus, v) != v) {
        emit playbackStatusChanged();
    }
}

qlonglong MediaPlayer2::position() const { return m_position; }

void MediaPlayer2::setPosition(qlonglong v) {
    if (std::exchange(m_position, v) != v) {
        emit positionChanged();
    }
}

double MediaPlayer2::rate() const { return m_rate; }

void MediaPlayer2::setRate(double v) {
    if (std::exchange(m_rate, v) != v) {
        emit rateChanged();
    }
}

bool MediaPlayer2::shuffle() const { return m_shuffle; }

void MediaPlayer2::setShuffle(bool v) {
    if (std::exchange(m_shuffle, v) != v) {
        emit shuffleChanged();
    }
}

double MediaPlayer2::volume() const { return m_volume; }

void MediaPlayer2::setVolume(double v) {
    if (std::exchange(m_volume, v) != v) {
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
