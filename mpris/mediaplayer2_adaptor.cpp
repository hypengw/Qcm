#include "mpris/mediaplayer2_adaptor.h"
#include "mpris/mediaplayer2.h"

#include <QMetaEnum>
#include <QUrl>

using namespace mpris;

MediaPlayer2Adaptor::MediaPlayer2Adaptor(MediaPlayer2* parent)
    : QDBusAbstractAdaptor(parent), m_realobj(parent) {
    // need signal to slot
    setAutoRelaySignals(false);

    using MP2  = MediaPlayer2;
    using MP2A = MediaPlayer2Adaptor;

    connect(parent, &MP2::canQuitChanged, this, &MP2A::onCanQuitChanged);
    connect(parent, &MP2::canRaiseChanged, this, &MP2A::onCanRaiseChanged);
    connect(parent, &MP2::canSetFullscreenChanged, this, &MP2A::onCanSetFullscreenChanged);
    connect(parent, &MP2::desktopEntryChanged, this, &MP2A::onDesktopEntryChanged);
    connect(parent, &MP2::fullscreenChanged, this, &MP2A::onFullscreenChanged);
    connect(parent, &MP2::hasTrackListChanged, this, &MP2A::onHasTrackListChanged);
    connect(parent, &MP2::identityChanged, this, &MP2A::onIdentityChanged);
    connect(parent, &MP2::supportedUriSchemesChanged, this, &MP2A::onSupportedUriSchemesChanged);
    connect(parent, &MP2::supportedMimeTypesChanged, this, &MP2A::onSupportedMimeTypesChanged);
}

bool        MediaPlayer2Adaptor::CanQuit() const { return m_realobj->canQuit(); }
bool        MediaPlayer2Adaptor::CanRaise() const { return m_realobj->canRaise(); }
bool        MediaPlayer2Adaptor::CanSetFullscreen() const { return m_realobj->canSetFullscreen(); }
QString     MediaPlayer2Adaptor::DesktopEntry() const { return m_realobj->desktopEntry(); }
bool        MediaPlayer2Adaptor::Fullscreen() const { return m_realobj->fullscreen(); }
bool        MediaPlayer2Adaptor::HasTrackList() const { return m_realobj->hasTrackList(); }
QString     MediaPlayer2Adaptor::Identity() const { return m_realobj->identity(); }
QStringList MediaPlayer2Adaptor::SupportedMimeTypes() const {
    return m_realobj->supportedMimeTypes();
}

QStringList MediaPlayer2Adaptor::SupportedUriSchemes() const {
    return m_realobj->supportedUriSchemes();
}

void MediaPlayer2Adaptor::setFullscreen(bool value) {
    if (CanSetFullscreen()) {
        emit m_realobj->fullscreenRequested(value);
        return;
    }

    if (value) {
        qDebug() << Q_FUNC_INFO << "Requested to fullscreen, but not supported";
    } else {
        qDebug() << Q_FUNC_INFO << "Requested to unfullscreen, but not supported";
    }
}

void MediaPlayer2Adaptor::Quit() {
    if (CanQuit()) {
        emit m_realobj->quitRequested();
        return;
    }

    // sendErrorReply(QDBusError::NotSupported, QStringLiteral("Quit requested but not
    // supported."));
}

void MediaPlayer2Adaptor::Raise() {
    if (CanRaise()) {
        emit m_realobj->raiseRequested();
        return;
    }

    // sendErrorReply(QDBusError::NotSupported, QStringLiteral("Raise requested but not
    // supported."));
}

MediaPlayer2PlayerAdaptor::MediaPlayer2PlayerAdaptor(MediaPlayer2* parent)
    : QDBusAbstractAdaptor(parent), m_realobj(parent) {
    // need signal to slot
    setAutoRelaySignals(false);
    using MP2   = MediaPlayer2;
    using MP2PA = MediaPlayer2PlayerAdaptor;

    connect(parent, &MP2::canControlChanged, this, &MP2PA::onCanControlChanged);
    connect(parent, &MP2::canGoNextChanged, this, &MP2PA::onCanGoNextChanged);
    connect(parent, &MP2::canGoPreviousChanged, this, &MP2PA::onCanGoPreviousChanged);
    connect(parent, &MP2::canPauseChanged, this, &MP2PA::onCanPauseChanged);
    connect(parent, &MP2::canPlayChanged, this, &MP2PA::onCanPlayChanged);
    connect(parent, &MP2::canSeekChanged, this, &MP2PA::onCanSeekChanged);
    connect(parent, &MP2::loopStatusChanged, this, &MP2PA::onLoopStatusChanged);
    connect(parent, &MP2::maximumRateChanged, this, &MP2PA::onMaximumRateChanged);
    connect(parent, &MP2::metadataChanged, this, &MP2PA::onMetadataChanged);
    connect(parent, &MP2::minimumRateChanged, this, &MP2PA::onMinimumRateChanged);
    connect(parent, &MP2::playbackStatusChanged, this, &MP2PA::onPlaybackStatusChanged);
    // PositionChanged signal is not forwarded through DBus ...
    connect(parent, &MP2::rateChanged, this, &MP2PA::onRateChanged);
    connect(parent, &MP2::shuffleChanged, this, &MP2PA::onShuffleChanged);
    connect(parent, &MP2::volumeChanged, this, &MP2PA::onVolumeChanged);
    connect(parent, &MP2::seeked, this, &MP2PA::Seeked);
}

QString MediaPlayer2PlayerAdaptor::PlaybackStatus() const {
    auto meta = QMetaEnum::fromType<MediaPlayer2::Playback_Status>();
    return meta.valueToKey(m_realobj->playbackStatus());
}
QString MediaPlayer2PlayerAdaptor::LoopStatus() const {
    auto meta = QMetaEnum::fromType<MediaPlayer2::Loop_Status>();
    return meta.valueToKey(m_realobj->loopStatus());
}
double      MediaPlayer2PlayerAdaptor::Rate() const { return m_realobj->rate(); }
bool        MediaPlayer2PlayerAdaptor::Shuffle() const { return m_realobj->shuffle(); }
QVariantMap MediaPlayer2PlayerAdaptor::Metadata() const { return m_realobj->metadata(); }
double      MediaPlayer2PlayerAdaptor::Volume() const { return m_realobj->volume(); }
qlonglong   MediaPlayer2PlayerAdaptor::Position() const { return m_realobj->position(); }
double      MediaPlayer2PlayerAdaptor::MinimumRate() const { return m_realobj->minimumRate(); }
double      MediaPlayer2PlayerAdaptor::MaximumRate() const { return m_realobj->minimumRate(); }
bool        MediaPlayer2PlayerAdaptor::CanGoNext() const { return m_realobj->canGoNext(); }
bool        MediaPlayer2PlayerAdaptor::CanGoPrevious() const { return m_realobj->canGoPrevious(); }
bool        MediaPlayer2PlayerAdaptor::CanPlay() const { return m_realobj->canPlay(); }
bool        MediaPlayer2PlayerAdaptor::CanPause() const { return m_realobj->canPause(); }
bool        MediaPlayer2PlayerAdaptor::CanSeek() const { return m_realobj->canSeek(); }
bool        MediaPlayer2PlayerAdaptor::CanControl() const { return m_realobj->canControl(); }

void MediaPlayer2PlayerAdaptor::setLoopStatus(QString v) {
    auto meta = QMetaEnum::fromType<MediaPlayer2::Loop_Status>();
    if (CanControl()) {
        bool ok;
        auto v_ = (MediaPlayer2::Loop_Status)meta.keyToValue(v.toLatin1(), &ok);
        emit m_realobj->loopStatusRequested(v_);
    }
}
void MediaPlayer2PlayerAdaptor::setRate(double v) {
    if (CanControl()) {
        emit m_realobj->rateRequested(v);
    }
}
void MediaPlayer2PlayerAdaptor::setShuffle(bool v) {
    if (CanControl()) {
        emit m_realobj->shuffleRequested(v);
    }
}
void MediaPlayer2PlayerAdaptor::setVolume(double v) {
    if (CanControl()) {
        emit m_realobj->volumeRequested(v);
    }
}

// Methods
void MediaPlayer2PlayerAdaptor::Next() { emit m_realobj->nextRequested(); }
void MediaPlayer2PlayerAdaptor::Previous() { emit m_realobj->previousRequested(); }
void MediaPlayer2PlayerAdaptor::Pause() { emit m_realobj->pauseRequested(); }
void MediaPlayer2PlayerAdaptor::PlayPause() { emit m_realobj->playPauseRequested(); }
void MediaPlayer2PlayerAdaptor::Stop() { emit m_realobj->stopRequested(); }
void MediaPlayer2PlayerAdaptor::Play() { emit m_realobj->playRequested(); }
void MediaPlayer2PlayerAdaptor::Seek(qlonglong v) { emit m_realobj->seekRequested(v); }
void MediaPlayer2PlayerAdaptor::SetPosition(const QDBusObjectPath& id, qlonglong v) {
    emit m_realobj->setPositionRequested(id, v);
}
void MediaPlayer2PlayerAdaptor::OpenUri(const QString& v) { emit m_realobj->openUriRequested(v); }

#define CON(_type_, _prop_)                                                                  \
    void _type_::on##_prop_##Changed() const {                                               \
        QVariantMap changedProperties;                                                       \
        auto        meta = _type_::metaObject();                                             \
        QString     interfaceName =                                                          \
            meta->classInfo(meta->indexOfClassInfo("D-Bus Interface")).value();              \
        changedProperties[QStringLiteral(#_prop_)] = QVariant(_prop_());                     \
        m_realobj->notifyPropertiesChanged(interfaceName, changedProperties, QStringList()); \
    }

CON(MediaPlayer2Adaptor, CanQuit);
CON(MediaPlayer2Adaptor, CanRaise);
CON(MediaPlayer2Adaptor, CanSetFullscreen);
CON(MediaPlayer2Adaptor, DesktopEntry);
CON(MediaPlayer2Adaptor, Fullscreen);
CON(MediaPlayer2Adaptor, HasTrackList);
CON(MediaPlayer2Adaptor, Identity);
CON(MediaPlayer2Adaptor, SupportedMimeTypes);
CON(MediaPlayer2Adaptor, SupportedUriSchemes);

CON(MediaPlayer2PlayerAdaptor, PlaybackStatus);
CON(MediaPlayer2PlayerAdaptor, LoopStatus);
CON(MediaPlayer2PlayerAdaptor, Rate);
CON(MediaPlayer2PlayerAdaptor, Shuffle);
CON(MediaPlayer2PlayerAdaptor, Metadata);
CON(MediaPlayer2PlayerAdaptor, Volume);
CON(MediaPlayer2PlayerAdaptor, MinimumRate);
CON(MediaPlayer2PlayerAdaptor, MaximumRate);
CON(MediaPlayer2PlayerAdaptor, CanGoNext);
CON(MediaPlayer2PlayerAdaptor, CanGoPrevious);
CON(MediaPlayer2PlayerAdaptor, CanPlay);
CON(MediaPlayer2PlayerAdaptor, CanPause);
CON(MediaPlayer2PlayerAdaptor, CanSeek);
CON(MediaPlayer2PlayerAdaptor, CanControl);


#include <mpris/moc_mediaplayer2_adaptor.cpp>