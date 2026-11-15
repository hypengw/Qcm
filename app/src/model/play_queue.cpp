module;
#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <QtCore/qnamespace.h>
#include <QtProtobuf/qtprotobuftypes.h>
#include "Qcm/model/play_queue.moc.h"
module qcm;
import rstd;
import :queue;
import :backend;

namespace qcm
{
using QueueAction  = msg::PlayerQueueActionGadget::PlayerQueueAction;
using PlayerAction = msg::PlayerActionGadget::PlayerAction;
using WireLoop     = msg::PlayerLoopModeGadget::PlayerLoopMode;

PlayQueue::PlayQueue(QObject* parent): QAbstractListModel(parent) {
    m_state.setCurrentIndex(-1);
    updateRoleNames(Song::staticMetaObject, this, {});
    m_subscription = AppStore::instance()->songs.subscribe([this](i64 id, quint64) {
        if (! m_state.songIds().contains(QtProtobuf::int64(id))) return;
        if (auto item = AppStore::instance()->songs.store_item(id))
            m_songs.insert_or_assign(model::ItemId(enums::ItemType::ItemSong, id), *item);
        notifyRowsForIds({ model::ItemId(enums::ItemType::ItemSong, id) });
    });
}
PlayQueue::~PlayQueue() { drop_global(); }

void PlayQueue::bindBackend(Backend* backend) {
    if (m_backend == backend) return;
    if (m_backend) disconnect(m_backend, nullptr, this, nullptr);
    m_backend = backend;
    connect(backend, &Backend::playerQueueChanged, this, &PlayQueue::applyState);
    connect(backend, &Backend::playerStateChanged, this, [this](const msg::PlayerStateMsg& state) {
        if (m_has_player_snapshot && state.revision() <= m_player_state.revision()) return;
        const bool changed = ! m_has_player_snapshot || state.songId() != m_player_state.songId() ||
                             state.title() != m_player_state.title() ||
                             state.durationMs() != m_player_state.durationMs();
        m_player_state     = state;
        m_has_player_snapshot = true;
        if (changed) currentSongChanged();
    });
    connect(backend, &Backend::connected, this, [this] {
        disconnectQueue();
    });
    connect(backend, &Backend::disconnected, this, &PlayQueue::disconnectQueue);
    connect(backend, &Backend::error, this, [this] {
        disconnectQueue();
    });
    connect(backend, &Backend::resourceStateChanged, this, [this] {
        if (m_backend && ! m_backend->resource_base_url().isEmpty()) reloadEntities();
    });
}

void PlayQueue::disconnectQueue() {
    ++m_entity_generation;
    m_loads.clear();
    m_loading_ids.clear();
    m_pending_ids.clear();
    m_has_snapshot        = false;
    m_has_player_snapshot = false;
    m_available           = false;
    m_mutating            = false;
    for (auto* radio : m_radios) radio->disconnectQueue();
    canNextChanged();
    canPrevChanged();
    canJumpChanged();
    canRemoveChanged();
    changingTrackChanged();
}

void PlayQueue::applyState(const msg::PlayerQueueMsg& state) {
    if (m_has_snapshot && state.revision() <= m_state.revision()) return;
    const auto old_index = currentIndex();
    const auto old_entry = currentEntryId();
    const auto old_radio = radioQueue();
    const bool structure_changed =
        m_state.songIds() != state.songIds() || m_state.entryIds() != state.entryIds();
    if (structure_changed) beginResetModel();
    m_state        = state;
    m_has_snapshot = true;
    m_available    = true;
    m_mutating     = false;
    if (structure_changed) endResetModel();

    std::unordered_set<model::ItemId> retained;
    for (auto id : m_state.songIds()) retained.emplace(enums::ItemType::ItemSong, qint64(id));
    std::erase_if(m_songs, [&](const auto& item) {
        return ! retained.contains(item.first);
    });
    std::erase_if(m_pending_ids, [&](const auto& id) {
        return ! retained.contains(id);
    });
    auto* store = AppStore::instance();
    for (const auto& id : retained) {
        if (auto item = store->songs.store_item(id.id()))
            m_songs.insert_or_assign(id, *item);
        else if (! std::ranges::any_of(m_loading_ids, [&](const auto& load) {
                     return load.second.contains(id);
                 }))
            m_pending_ids.insert(id);
    }
    if (identity::valid(state.radioQueueId())) {
        const qint64 id = qint64(state.radioQueueId().value());
        if (! m_radios.contains(id))
            m_radios.insert(id, new model::DynamicIdQueue(state.radioQueueId(), this));
        connect(m_radios.value(id),
                &model::DynamicIdQueue::configuringChanged,
                this,
                &PlayQueue::changingTrackChanged,
                Qt::UniqueConnection);
        m_radios.value(id)->applyState(state);
    }
    if (old_index != currentIndex()) currentIndexChanged(currentIndex());
    if (old_entry != currentEntryId()) currentEntryChanged();
    currentSongChanged();
    loopModeChanged(loopMode());
    randomModeChanged(randomMode());
    canNextChanged();
    canPrevChanged();
    canJumpChanged();
    canRemoveChanged();
    nameChanged();
    if (old_radio != radioQueue()) radioQueueChanged();
    changingTrackChanged();
    fetchSongs();
}

void PlayQueue::drop_global() {
    if (m_backend) disconnect(m_backend, nullptr, this, nullptr);
    m_backend   = nullptr;
    m_available = false;
    ++m_entity_generation;
    m_subscription.reset();
    m_loads.clear();
    m_loading_ids.clear();
    m_pending_ids.clear();
    m_songs.clear();
}

auto PlayQueue::rowCount(const QModelIndex& parent) const -> int {
    return parent.isValid() ? 0 : int(m_state.songIds().size());
}
auto PlayQueue::roleNames() const -> QHash<int, QByteArray> {
    auto roles = roleNamesRef();
    roles.insert(model::ItemIdListModel::ItemIdRole, "itemId");
    roles.insert(model::ItemIdListModel::ItemIdRole + 1, "entryId");
    return roles;
}
auto PlayQueue::data(const QModelIndex& index, int role) const -> QVariant {
    if (! index.isValid() || index.model() != this) return {};
    auto id = getId(index.row());
    if (! id) return {};
    if (role == Qt::DisplayRole || role == model::ItemIdListModel::ItemIdRole)
        return QVariant::fromValue(*id);
    if (role == model::ItemIdListModel::ItemIdRole + 1) {
        const auto entries = m_state.entryIds();
        return index.row() < entries.size()
                   ? QVariant(QString::number(qulonglong(entries[index.row()])))
                   : QVariant();
    }
    if (auto property = propertyOfRole(role)) {
        auto found = m_songs.find(*id);
        if (found != m_songs.end())
            if (auto* song = found->second.item()) return property->readOnGadget(song);
        Song placeholder;
        model::set_item_id(placeholder, *id);
        if (*id == currentSongId()) placeholder = currentSong();
        return property->readOnGadget(&placeholder);
    }
    return {};
}

auto PlayQueue::currentIndex() const -> qint32 {
    const int index = int(m_state.currentIndex());
    return index >= 0 && index < rowCount() ? index : -1;
}
auto PlayQueue::currentEntryId() const -> QString {
    return currentIndex() >= 0 ? QString::number(qulonglong(m_state.currentEntryId())) : QString();
}
auto PlayQueue::currentId() const -> rstd::Option<model::ItemId> { return getId(currentIndex()); }
auto PlayQueue::getId(qint32 index) const -> rstd::Option<model::ItemId> {
    const auto ids = m_state.songIds();
    if (index < 0 || index >= ids.size()) return rstd::None();
    return rstd::Some(model::ItemId(enums::ItemType::ItemSong, qint64(ids[index])));
}
auto PlayQueue::currentSongId() const -> model::ItemId {
    return currentId().unwrap_or(model::ItemId {});
}
auto PlayQueue::currentSong() const -> Song {
    const auto id = currentSongId();
    if (auto found = m_songs.find(id); found != m_songs.end())
        if (auto* song = found->second.item()) return *song;
    Song result;
    model::set_item_id(result, id);
    if (auto song_id = id.toSongId(); song_id && *song_id == m_player_state.songId()) {
        result.setName(m_player_state.title());
        result.setDuration(double(qint64(m_player_state.durationMs())));
    }
    return result;
}
auto PlayQueue::currentAlbumId() const -> model::ItemId {
    return model::album_item_id(currentSong());
}
auto PlayQueue::changingTrack() const -> bool {
    if (! m_available) return false;
    if (auto radio = radioQueue(); radio && radio->configuring()) return true;
    auto id = currentId();
    if (! id) return false;
    if (auto song = m_songs.find(*id); song != m_songs.end() && song->second.item()) return false;
    return m_pending_ids.contains(*id) || std::ranges::any_of(m_loading_ids, [&](const auto& load) {
               return load.second.contains(*id);
           });
}
auto PlayQueue::loopMode() const -> LoopMode {
    if (m_state.shuffle()) return LoopMode::ShuffleLoop;
    switch (m_state.loopMode()) {
    case WireLoop::PLAYER_LOOP_MODE_TRACK: return LoopMode::SingleLoop;
    case WireLoop::PLAYER_LOOP_MODE_PLAYLIST: return LoopMode::ListLoop;
    default: return LoopMode::NoneLoop;
    }
}
auto PlayQueue::randomMode() const -> bool { return m_state.randomMode(); }
auto PlayQueue::canNext() const -> bool { return m_available && m_state.canNext(); }
auto PlayQueue::canPrev() const -> bool { return m_available && m_state.canPrevious(); }
auto PlayQueue::canJump() const -> bool { return m_available && m_state.canJump(); }
auto PlayQueue::canRemove() const -> bool { return m_available && m_state.canRemove(); }
auto PlayQueue::name() const -> QString { return m_state.name(); }
auto PlayQueue::radioQueue() const -> model::DynamicIdQueue* {
    return identity::valid(m_state.radioQueueId())
               ? m_radios.value(qint64(m_state.radioQueueId().value()))
               : nullptr;
}

void PlayQueue::sendRequest(msg::PlayerQueueReq request, std::function<void(bool)> completed) {
    if (! m_available || ! m_backend) {
        if (completed) completed(false);
        return;
    }
    request.setExpectedRevision(m_state.revision());
    m_backend->updatePlayerQueue(std::move(request), std::move(completed));
}
void PlayQueue::sendSongs(QueueAction action, std::span<const model::ItemId> songs, bool play,
                          std::function<void(bool)> completed) {
    QtProtobuf::int64List ids;
    for (const auto& song : songs)
        if (auto id = song.toSongId()) ids.push_back(QtProtobuf::int64(qint64(id->value())));
    if (ids.isEmpty()) {
        if (completed) completed(false);
        return;
    }
    msg::PlayerQueueReq request;
    request.setAction(action);
    request.setSongIds(ids);
    request.setPlay(play);
    sendRequest(std::move(request), std::move(completed));
}
void PlayQueue::playSong(model::ItemId song, std::function<void(bool)> completed) {
    if (! song.toSongId()) {
        if (completed) completed(false);
        return;
    }
    const auto ids = m_state.songIds();
    if (! radioQueue()) {
        auto index = ids.indexOf(QtProtobuf::int64(song.id()));
        if (index >= 0) {
            msg::PlayerQueueReq request;
            request.setAction(QueueAction::PLAYER_QUEUE_ACTION_SELECT);
            request.setIndex(int(index));
            request.setPlay(true);
            sendRequest(std::move(request), std::move(completed));
            return;
        }
    }
    const std::array songs { song };
    sendSongs(QueueAction::PLAYER_QUEUE_ACTION_APPEND, songs, true, std::move(completed));
}
void PlayQueue::replaceSongs(std::span<const model::ItemId> songs,
                             std::function<void(bool)>      completed) {
    sendSongs(QueueAction::PLAYER_QUEUE_ACTION_REPLACE, songs, true, std::move(completed));
}
void PlayQueue::appendSongs(std::span<const model::ItemId> songs) {
    sendSongs(QueueAction::PLAYER_QUEUE_ACTION_APPEND, songs, false);
}
void PlayQueue::insertNext(std::span<const model::ItemId> songs) {
    sendSongs(QueueAction::PLAYER_QUEUE_ACTION_INSERT_NEXT, songs, false);
}

void PlayQueue::next() {
    if (! canNext()) return;
    msg::PlayerControlReq request;
    request.setAction(PlayerAction::PLAYER_ACTION_NEXT);
    m_backend->controlPlayer(std::move(request));
}
void PlayQueue::prev() {
    if (! canPrev()) return;
    msg::PlayerControlReq request;
    request.setAction(PlayerAction::PLAYER_ACTION_PREVIOUS);
    m_backend->controlPlayer(std::move(request));
}
void PlayQueue::setLoopMode(LoopMode mode) {
    msg::PlayerQueueReq request;
    request.setAction(QueueAction::PLAYER_QUEUE_ACTION_LOOP);
    request.setShuffle(mode == LoopMode::ShuffleLoop);
    request.setLoopMode(mode == LoopMode::SingleLoop ? WireLoop::PLAYER_LOOP_MODE_TRACK
                        : mode == LoopMode::NoneLoop ? WireLoop::PLAYER_LOOP_MODE_NONE
                                                     : WireLoop::PLAYER_LOOP_MODE_PLAYLIST);
    sendRequest(std::move(request));
}
void PlayQueue::iterLoopMode() {
    switch (loopMode()) {
    case LoopMode::NoneLoop: setLoopMode(LoopMode::SingleLoop); break;
    case LoopMode::SingleLoop: setLoopMode(LoopMode::ListLoop); break;
    case LoopMode::ListLoop: setLoopMode(LoopMode::ShuffleLoop); break;
    case LoopMode::ShuffleLoop: setLoopMode(LoopMode::NoneLoop); break;
    }
}
void PlayQueue::setRandomMode(bool value) {
    msg::PlayerQueueReq request;
    request.setAction(QueueAction::PLAYER_QUEUE_ACTION_RANDOM);
    request.setRandomMode(value);
    sendRequest(std::move(request));
}
void PlayQueue::select(qint32 index) {
    if (! canJump() || index < 0 || index >= rowCount()) return;
    msg::PlayerQueueReq request;
    request.setAction(QueueAction::PLAYER_QUEUE_ACTION_SELECT);
    request.setIndex(index);
    request.setPlay(true);
    sendRequest(std::move(request));
}
void PlayQueue::clear() {
    if (! canRemove()) return;
    msg::PlayerQueueReq request;
    request.setAction(QueueAction::PLAYER_QUEUE_ACTION_CLEAR);
    sendRequest(std::move(request));
}
bool PlayQueue::removeRows(int row, int count, const QModelIndex& parent) {
    if (parent.isValid() || ! canRemove() || count <= 0 || row < 0 || row > rowCount() - count)
        return false;
    msg::PlayerQueueReq request;
    request.setAction(QueueAction::PLAYER_QUEUE_ACTION_REMOVE);
    request.setIndex(row);
    request.setCount(count);
    sendRequest(std::move(request));
    return true;
}
bool PlayQueue::moveRows(const QModelIndex& parent, int row, int count,
                         const QModelIndex& destination_parent, int destination) {
    if (parent.isValid() || destination_parent.isValid() || ! canRemove() || m_mutating ||
        row < 0 || count <= 0 || row > rowCount() - count || destination < 0 ||
        destination > rowCount() || (destination >= row && destination <= row + count))
        return false;
    msg::PlayerQueueReq request;
    request.setAction(QueueAction::PLAYER_QUEUE_ACTION_MOVE);
    request.setIndex(row);
    request.setCount(count);
    request.setDestination(destination);
    m_mutating = true;
    auto self  = QPointer<PlayQueue>(this);
    sendRequest(std::move(request), [self](bool) {
        if (self) self->m_mutating = false;
    });
    return true;
}
bool PlayQueue::move(qint32 source, qint32 destination, qint32 count) {
    if (source == destination) return false;
    if (source < destination) destination += count;
    return moveRows({}, source, count, {}, destination);
}

auto PlayQueue::dynamicQueue(const msg::id::RadioQueueId& id, const QString& name,
                             const QList<msg::model::QueueConfigField>& configuration,
                             bool can_restart, const QString& restart_label)
    -> model::DynamicIdQueue* {
    if (! identity::valid(id)) return nullptr;
    auto* radio = m_radios.value(qint64(id.value()));
    if (! radio) {
        radio = new model::DynamicIdQueue(id, this);
        m_radios.insert(qint64(id.value()), radio);
    }
    connect(radio,
            &model::DynamicIdQueue::configuringChanged,
            this,
            &PlayQueue::changingTrackChanged,
            Qt::UniqueConnection);
    radio->updateDefinition(name, configuration, can_restart, restart_label);
    return radio;
}

void PlayQueue::reloadEntities() {
    ++m_entity_generation;
    m_loads.clear();
    m_loading_ids.clear();
    m_pending_ids.clear();
    for (auto id : m_state.songIds()) m_pending_ids.emplace(enums::ItemType::ItemSong, qint64(id));
    fetchSongs();
}
void PlayQueue::notifyRowsForIds(const std::unordered_set<model::ItemId>& ids) {
    for (int row = 0; row < rowCount(); ++row)
        if (auto id = getId(row); id && ids.contains(*id)) dataChanged(index(row), index(row));
    if (auto id = currentId(); id && ids.contains(*id)) currentSongChanged();
    changingTrackChanged();
}
void PlayQueue::fetchSongs() {
    if (! m_backend || m_backend->resource_base_url().isEmpty()) return;
    while (! m_pending_ids.empty() && m_loads.size() < 8) {
        QList<i64>                        ids;
        std::unordered_set<model::ItemId> loading;
        for (auto it = m_pending_ids.begin(); it != m_pending_ids.end() && ids.size() < 512;) {
            ids.push_back(it->id());
            loading.insert(*it);
            it = m_pending_ids.erase(it);
        }
        const auto ticket     = ++m_load_serial;
        const auto generation = m_entity_generation;
        m_loading_ids.emplace(ticket, std::move(loading));
        changingTrackChanged();
        auto self  = QPointer<PlayQueue>(this);
        auto lease = m_backend->loadSongs(
            identity::fromValues<msg::id::SongIds>(ids),
            this,
            [self, ticket, generation](const Backend::SongResult& response) {
                if (! self || self->m_entity_generation != generation) return;
                self->m_loads.erase(ticket);
                self->m_loading_ids.erase(ticket);
                if (response) {
                    std::unordered_set<model::ItemId> fetched;
                    auto*                             store = AppStore::instance();
                    for (auto id : response->ids().values()) {
                        const qint64 song_id = qint64(id);
                        if (! self->m_state.songIds().contains(QtProtobuf::int64(song_id)))
                            continue;
                        model::ItemId key(enums::ItemType::ItemSong, song_id);
                        if (auto item = store->songs.store_item(song_id)) {
                            fetched.insert(key);
                            self->m_songs.insert_or_assign(key, *item);
                        }
                    }
                    self->notifyRowsForIds(fetched);
                }
                self->changingTrackChanged();
                self->fetchSongs();
            });
        if (lease)
            m_loads.emplace(ticket, std::move(lease));
        else
            m_loading_ids.erase(ticket);
    }
}
} // namespace qcm
