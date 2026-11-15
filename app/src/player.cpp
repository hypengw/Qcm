module;
#include <algorithm>
#include <cmath>
#include "Qcm/player.moc.h"

module qcm;
import :player;
import :backend;

namespace qcm
{
using PlayerAction = msg::PlayerActionGadget::PlayerAction;

Player::Player(QObject* parent): QObject(parent) {
    m_state.setPlaybackState(int(PlaybackState::StoppedState));
    m_state.setVolume(1.0);
}

void Player::bindBackend(Backend* backend) {
    if (m_backend == backend) return;
    if (m_backend) disconnect(m_backend, nullptr, this, nullptr);
    m_backend = backend;
    disconnectPlayer();
    connect(backend, &Backend::playerStateChanged, this, &Player::applyState);
    connect(backend, &Backend::connected, this, [this] {
        m_has_snapshot = false;
    });
    connect(backend, &Backend::disconnected, this, &Player::disconnectPlayer);
    connect(backend, &Backend::error, this, [this] {
        disconnectPlayer();
    });
}

void Player::disconnectPlayer() {
    m_has_snapshot = false;
    if (! std::exchange(m_connected, false)) return;
    connectedChanged();
    seekableChanged();
    busyChanged();
    playbackStateChanged();
}

void Player::applyState(const msg::PlayerStateMsg& value) {
    if (m_has_snapshot && value.revision() <= m_state.revision()) return;
    const auto old           = m_state;
    const bool had_snapshot  = m_has_snapshot;
    const bool old_connected = m_connected;
    m_state                  = value;
    m_has_snapshot           = true;
    m_connected              = value.connected();
    if (old_connected != m_connected) connectedChanged();
    if (! had_snapshot || old.source() != value.source() ||
        old.sourceEpoch() != value.sourceEpoch())
        sourceChanged();
    if (old.positionMs() != value.positionMs()) positionChanged();
    if (old.durationMs() != value.durationMs()) durationChanged();
    if (old.volume() != value.volume()) volumeChanged(volume());
    if (old.fadeMs() != value.fadeMs()) fadeTimeChanged(fadeTime());
    if (old.busy() != value.busy() || old_connected != m_connected) busyChanged();
    if (old.seekable() != value.seekable() || old_connected != m_connected) seekableChanged();
    if (old.playbackState() != value.playbackState() || old_connected != m_connected)
        playbackStateChanged();
    if (old.cacheBegin() != value.cacheBegin() || old.cacheEnd() != value.cacheEnd())
        cacheProgressChanged();
    if (had_snapshot && value.endedSerial() > old.endedSerial()) ended();
    if (had_snapshot && value.seekSerial() > old.seekSerial()) seeked(double(position()) * 1000);
    if (! value.error().isEmpty() && (! had_snapshot || old.error() != value.error()))
        errorOccurred(value.error());
}

auto Player::source() const -> QUrl { return QUrl(m_state.source()); }
auto Player::position() const -> qint64 { return qint64(m_state.positionMs()); }
auto Player::duration() const -> qint64 { return qint64(m_state.durationMs()); }
auto Player::busy() const -> bool { return m_connected && m_state.busy(); }
auto Player::changingTrack() const -> bool { return m_changing_track; }
void Player::setChangingTrack(bool value) {
    if (std::exchange(m_changing_track, value) != value) changingTrackChanged();
}
auto Player::playback_state() const -> PlaybackState {
    return static_cast<PlaybackState>(int(m_state.playbackState()));
}
auto Player::volume() const -> float { return float(m_state.volume()); }
auto Player::fadeTime() const -> u32 { return m_state.fadeMs(); }
auto Player::cache_progress() const -> QVector2D {
    return { float(m_state.cacheBegin()), float(m_state.cacheEnd()) };
}
auto Player::seekable() const -> bool { return m_connected && m_state.seekable(); }
auto Player::playing() const -> bool {
    return m_connected && playback_state() == PlaybackState::PlayingState;
}
auto Player::connected() const -> bool { return m_connected; }

void Player::command(PlayerAction action) {
    if (! m_backend) return;
    msg::PlayerControlReq request;
    request.setAction(action);
    m_backend->controlPlayer(std::move(request));
}
void Player::toggle() { command(PlayerAction::PLAYER_ACTION_TOGGLE); }
void Player::play() { command(PlayerAction::PLAYER_ACTION_PLAY); }
void Player::pause() { command(PlayerAction::PLAYER_ACTION_PAUSE); }
void Player::stop() { command(PlayerAction::PLAYER_ACTION_STOP); }

void Player::set_source(const QUrl& value) {
    if (! m_backend) return;
    msg::PlayerControlReq request;
    request.setAction(PlayerAction::PLAYER_ACTION_SOURCE);
    request.setSource(value.toString(QUrl::FullyEncoded));
    m_backend->controlPlayer(std::move(request));
}
void Player::reset_source() { set_source({}); }
void Player::set_position(qint64 value) {
    if (! m_backend || ! seekable() || duration() <= 0) return;
    msg::PlayerControlReq request;
    request.setAction(PlayerAction::PLAYER_ACTION_SEEK);
    request.setPositionMs(std::clamp(value, qint64(0), duration()));
    request.setExpectedSourceEpoch(m_state.sourceEpoch());
    m_backend->controlPlayer(std::move(request));
}
void Player::set_volume(float value) {
    if (! m_backend || ! std::isfinite(value)) return;
    msg::PlayerControlReq request;
    request.setAction(PlayerAction::PLAYER_ACTION_VOLUME);
    request.setVolume(std::clamp(value, 0.0f, 1.0f));
    m_backend->controlPlayer(std::move(request));
}
void Player::set_fadeTime(u32 value) {
    if (! m_backend) return;
    msg::PlayerControlReq request;
    request.setAction(PlayerAction::PLAYER_ACTION_FADE);
    request.setFadeMs(value);
    m_backend->controlPlayer(std::move(request));
}
void Player::seek(double value) {
    if (std::isfinite(value)) set_position(qint64(std::clamp(value, 0.0, 1.0) * duration()));
}
} // namespace qcm

#include "Qcm/player.moc.cpp"
