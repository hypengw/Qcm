module;
#include "Qcm/model/play_queue.moc.h"

#include "core/log.h"

module qcm;
import :queue;
import :app;
import qextra;
import qcm.log;

namespace qcm
{

PlayIdQueue::PlayIdQueue(QObject* parent): model::IdQueue(parent) { setOptions(Options(~0)); }
PlayIdQueue::~PlayIdQueue() {}

PlayIdProxyQueue::PlayIdProxyQueue(QObject* parent)
    : QIdentityProxyModel(parent), m_support_shuffle(true), m_current_index(-1), m_shuffle(true) {
    connect(this, &PlayIdProxyQueue::shuffleChanged, this, &PlayIdProxyQueue::reShuffle);
    connect(this, &PlayIdProxyQueue::layoutChanged, this, [this] {
        if (m_shuffle.hasBinding()) m_shuffle.notify();
    });
}

PlayIdProxyQueue::~PlayIdProxyQueue() {}
void PlayIdProxyQueue::setSourceModel(QAbstractItemModel* source_model) {
    auto old = sourceModel();
    if (old == source_model) return;

    QIdentityProxyModel::setSourceModel(source_model);
#ifdef _WIN32
    connect(source_model,
            SIGNAL(currentIndexChanged(qint32)),
            this,
            SLOT(setCurrentIndexFromSource(qint32)));
#else
    QBindable<qint32> source_idx(source_model, "currentIndex");
    m_current_index.setBinding([source_idx, this] {
        auto source = source_idx.value();
        return m_shuffle.value() && m_support_shuffle.value() ? mapFromSource(source) : source;
    });
#endif

    shuffleSync();

    auto options      = source_model->property("options").value<model::IdQueue::Options>();
    m_support_shuffle = options.testFlag(model::IdQueue::Option::SupportShuffle);
    if (old) {
        disconnect(
            old, &QAbstractItemModel::rowsInserted, this, &PlayIdProxyQueue::onSourceRowsInserted);
        disconnect(
            old, &QAbstractItemModel::rowsRemoved, this, &PlayIdProxyQueue::onSourceRowsRemoved);
        disconnect(old, &QAbstractItemModel::rowsMoved, this, &PlayIdProxyQueue::onSourceRowsMoved);
        disconnect(old,
                   &QAbstractItemModel::rowsAboutToBeInserted,
                   this,
                   &PlayIdProxyQueue::onSourceRowsAboutToBeInserted);
    }
    connect(source_model,
            &QAbstractItemModel::rowsInserted,
            this,
            &PlayIdProxyQueue::onSourceRowsInserted);
    connect(source_model,
            &QAbstractItemModel::rowsAboutToBeInserted,
            this,
            &PlayIdProxyQueue::onSourceRowsAboutToBeInserted);
    connect(source_model,
            &QAbstractItemModel::rowsRemoved,
            this,
            &PlayIdProxyQueue::onSourceRowsRemoved);
    connect(
        source_model, &QAbstractItemModel::rowsMoved, this, &PlayIdProxyQueue::onSourceRowsMoved);
}

auto PlayIdProxyQueue::shuffle() const -> bool { return m_shuffle.value(); }
void PlayIdProxyQueue::setShuffle(bool v) { m_shuffle = v; }
auto PlayIdProxyQueue::bindableShuffle() -> QBindable<bool> { return &m_shuffle; }
auto PlayIdProxyQueue::useShuffle() const -> bool { return m_support_shuffle && shuffle(); }

auto PlayIdProxyQueue::currentIndex() const -> qint32 { return m_current_index.value(); }
void PlayIdProxyQueue::setCurrentIndex(qint32 idx) {
    sourceModel()->setProperty("currentIndex", mapToSource(idx));
}
void PlayIdProxyQueue::setCurrentIndexFromSource(qint32 source) {
    m_current_index =
        m_shuffle.value() && m_support_shuffle.value() ? mapFromSource(source) : source;
}
auto PlayIdProxyQueue::bindableCurrentIndex() -> const QBindable<qint32> {
    return &m_current_index;
}
auto PlayIdProxyQueue::randomIndex() -> int {
    auto cur   = currentIndex();
    auto count = rowCount();
    if (count <= 1) return cur;

    int out = cur;
    while (out == cur) {
        out = Random::get(0, count - 1);
    }
    setCurrentIndex(out);
    return out;
}

auto PlayIdProxyQueue::mapToSource(const QModelIndex& proxy_index) const -> QModelIndex {
    if (! proxy_index.isValid()) return QModelIndex();
    return sourceModel()->index(mapToSource(proxy_index.row()), proxy_index.column());
}

auto PlayIdProxyQueue::mapFromSource(const QModelIndex& sourc_index) const -> QModelIndex {
    if (! sourc_index.isValid()) return QModelIndex();
    return index(mapFromSource(sourc_index.row()), sourc_index.column());
}

auto PlayIdProxyQueue::mapToSource(int row) const -> int {
    if (row < 0 || row >= (int)m_shuffle_list.size()) return -1;
    return useShuffle() ? m_shuffle_list[row] : row;
}

auto PlayIdProxyQueue::mapFromSource(int row) const -> int {
    int proxy_row { -1 };
    if (useShuffle()) {
        if (auto it = m_source_to_proxy.find(row); it != m_source_to_proxy.end()) {
            proxy_row = it->second;
        } else {
            proxy_row = -1;
        }
    } else {
        proxy_row = row;
    }
    return proxy_row;
}

void PlayIdProxyQueue::reShuffle() {
    layoutAboutToBeChanged();
    if (useShuffle()) {
        Random::shuffle(m_shuffle_list.begin(), m_shuffle_list.end());
        if (auto it = std::find(m_shuffle_list.begin(), m_shuffle_list.end(), 0);
            it != m_shuffle_list.end()) {
            std::swap(*it, m_shuffle_list.front());
        }
        refreshFromSource();
    }
    layoutChanged();
}

void PlayIdProxyQueue::shuffleSync() {
    layoutAboutToBeChanged();
    auto count = sourceModel()->rowCount();
    auto old   = (int)m_shuffle_list.size();
    if (old < count) {
        while ((int)m_shuffle_list.size() < count) m_shuffle_list.push_back(m_shuffle_list.size());

        auto cur = m_current_index.value();
        Random::shuffle(m_shuffle_list.begin() + cur + 1, m_shuffle_list.end());
        refreshFromSource();

    } else if (old > count) {
        for (int i = 0, k = 1; i < count; i++) {
            if (m_shuffle_list[i] >= count) {
                std::swap(m_shuffle_list[i], m_shuffle_list[old - k]);
                k++;
            }
        }
        m_shuffle_list.resize(count);
        refreshFromSource();

        // do not need check cur here
        // let upstream check later
        // if upstream no change, cur is ok
    }
    layoutChanged();
}

void PlayIdProxyQueue::refreshFromSource() {
    int rowCount = sourceModel()->rowCount();
    m_source_to_proxy.clear();
    for (int proxyRow = 0; proxyRow < rowCount; ++proxyRow) {
        int sourceRow                = m_shuffle_list[proxyRow];
        m_source_to_proxy[sourceRow] = proxyRow;
    }
}

void PlayIdProxyQueue::onSourceRowsInserted(const QModelIndex&, int, int) { shuffleSync(); }
void PlayIdProxyQueue::onSourceRowsRemoved(const QModelIndex&, int, int) { shuffleSync(); }
void PlayIdProxyQueue::onSourceRowsMoved(const QModelIndex&, int, int, const QModelIndex&, int) {
    shuffleSync();
}

void PlayIdProxyQueue::onSourceRowsAboutToBeInserted(const QModelIndex&, int, int) {}

PlayQueue::PlayQueue(QObject* parent)
    : QIdentityProxyModel(parent),
      m_proxy(new PlayIdProxyQueue(parent)),
      m_loop_mode(LoopMode::NoneLoop),
      m_can_next(false),
      m_can_prev(false),
      m_can_jump(true),
      m_can_user_remove(true),
      m_random_mode(false),
      m_changed_timer(new QTimer(this)),
      m_pending_advance_timer(new QTimer(this)),
      m_pending_advance(false) {
    updateRoleNames(qcm::model::Song::staticMetaObject, this, {});
    connect(this, &PlayQueue::currentIndexChanged, this, [this](qint32 idx) {
        setCurrentSong(idx);

        LOG_DEBUG("queue: current index changed to {}, id {}",
                  idx,
                  m_current_song.as_ref()
                      .and_then([](auto& el) -> rstd::Option<i64> {
                          return rstd::into(el.key());
                      })
                      .unwrap_or(-1));
    });
    connect(m_proxy, &PlayIdProxyQueue::currentIndexChanged, this, &PlayQueue::checkCanMove);
    connect(this, &PlayQueue::loopModeChanged, m_proxy, [this] {
        m_proxy->setShuffle(loopMode() == LoopMode::ShuffleLoop);
        checkCanMove();
    });
    connect(this, &PlayQueue::pendingIdsChanged, this, &PlayQueue::fetchSongs);
    connect(m_changed_timer, &QTimer::timeout, this, [this]() {
        if (! sourceModel()) return;
        if (m_changed_ids.empty()) return;

        std::unordered_set<model::ItemId> ids { m_changed_ids.begin(), m_changed_ids.end() };
        m_changed_ids.clear();
        notifyRowsForIds(ids);
    });

    m_changed_timer->setSingleShot(true);
    m_changed_timer->setInterval(200);

    m_pending_advance_timer->setSingleShot(true);
    m_pending_advance_timer->setInterval(5000);
    connect(m_pending_advance_timer, &QTimer::timeout, this, [this]() {
        m_pending_advance = false;
    });
    m_subscription = AppStore::instance()->songs.subscribe([this](i64 id, quint64) {
        std::vector<model::ItemId> changed_ids { model::ItemId(enums::ItemType::ItemSong, id) };
        addChangedId(changed_ids);
    });

    loopModeChanged(m_loop_mode);
}
PlayQueue::~PlayQueue() { m_background_tasks.cancel(); }
void PlayQueue::drop_global() {
    m_subscription.reset();
    m_loads.clear();
    m_songs.clear();
}

auto PlayQueue::roleNames() const -> QHash<int, QByteArray> {
    auto roles = roleNamesRef();
    roles.insert(model::ItemIdListModel::ItemIdRole, "itemId");
    return roles;
}
auto PlayQueue::data(const QModelIndex& index, int role) const -> QVariant {
    auto row = index.row();
    auto id  = getId(row);
    if (role == model::ItemIdListModel::ItemIdRole) {
        if (id) return QVariant::fromValue(*id);
        return {};
    }
    do {
        if (! id) break;
        auto it    = m_songs.find(*id);
        bool it_ok = it != m_songs.end();

        if (auto prop = this->propertyOfRole(role); prop) {
            if (it_ok) {
                auto* song = it->second.item();
                if (song) {
                    return prop.value().readOnGadget(song);
                } else {
                    return prop.value().readOnGadget(&m_placeholder);
                }
            } else if (prop->name() == "sourceId"sv) {
                // if (auto it = m_source_map.find(hash); it != m_source_map.end()) {
                //     return QVariant::fromValue(it->second);
                // } else {
                //     return prop.value().readOnGadget(&m_placeholder);
                // }
            } else {
                return prop.value().readOnGadget(&m_placeholder);
            }
        }
    } while (0);
    return {};
}

void PlayQueue::setSourceModel(QAbstractItemModel* source_model) {
    auto old = sourceModel();

    if (old == source_model) return;

    QIdentityProxyModel::setSourceModel(source_model);
#ifdef _WIN32
    connect(source_model, SIGNAL(currentIndexChanged(qint32)), this, SLOT(setCurrentIndex(qint32)));
#else
    QBindable<qint32> source_idx(source_model, "currentIndex");
    m_current_index.setBinding([source_idx] {
        return source_idx.value();
    });
#endif

    if (old) {
        disconnect(old, &QAbstractItemModel::rowsInserted, this, &PlayQueue::onSourceRowsInserted);
        disconnect(old, &QAbstractItemModel::rowsRemoved, this, &PlayQueue::onSourceRowsRemoved);
        disconnect(old,
                   &QAbstractItemModel::rowsAboutToBeRemoved,
                   this,
                   &PlayQueue::onSourceRowsAboutToBeRemoved);
        disconnect(this, SIGNAL(requestNext()), old, SIGNAL(requestNext()));
    }
    connect(this, SIGNAL(requestNext()), source_model, SIGNAL(requestNext()));
    connect(
        source_model, &QAbstractItemModel::rowsInserted, this, &PlayQueue::onSourceRowsInserted);
    connect(source_model, &QAbstractItemModel::rowsRemoved, this, &PlayQueue::onSourceRowsRemoved);
    connect(source_model,
            &QAbstractItemModel::rowsAboutToBeRemoved,
            this,
            &PlayQueue::onSourceRowsAboutToBeRemoved);

    m_options = source_model->property("options").value<model::IdQueue::Options>();
    m_proxy->setSourceModel(source_model);
    checkCanMove();

    if (old) {
        this->onSourceRowsInserted({}, 0, source_model->rowCount());
    }

    if (auto p = source_model->property("name"); p.isValid()) {
        m_name = p.toString();
        nameChanged();
    }

    setCanJump(m_options & Option::SupportJump);
    setCanRemove(m_options & Option::SupportUserRemove);
    // A source switch can select a different song at the same row.
    setCurrentSong(currentIndex());
    radioQueueChanged();
}

auto PlayQueue::currentSong() const -> model::Song {
    if (m_current_song) {
        if (auto ptr = m_current_song->item()) {
            return *ptr;
        } else {
            LOG_ERROR("no such song: {}", m_current_song->key().value_or(-1));
        }
    }

    model::Song s {};
    if (auto id = currentId()) {
        model::set_item_id(s, *id);
        if (auto it = m_source_map.find(*id); it != m_source_map.end()) {
            // s.sourceId = it->second;
        }
    }
    return s;
}

auto PlayQueue::currentSongId() const -> model::ItemId {
    return currentId().unwrap_or(model::ItemId {});
}

auto PlayQueue::currentAlbumId() const -> model::ItemId {
    if (m_current_song) {
        if (auto ptr = m_current_song->item()) return model::album_item_id(*ptr);
    }
    return {};
}

void PlayQueue::setCurrentSong(rstd::Option<SongItem> s) {
    if (m_current_song != s) {
        m_current_song = std::move(s);
    }
    // Store handles compare identity; their metadata can change in place.
    currentSongChanged();
}
void PlayQueue::setCurrentSong(qint32 idx) {
    setCurrentSong(getId(idx).and_then([this](const auto& id) -> rstd::Option<SongItem> {
        if (auto it = m_songs.find(id); it != m_songs.end()) {
            return Some(SongItem { it->second });
        }
        return None();
    }));
}

auto PlayQueue::name() const -> const QString& { return m_name; }
auto PlayQueue::currentId() const -> rstd::Option<model::ItemId> { return getId(currentIndex()); }

auto PlayQueue::getId(qint32 idx) const -> rstd::Option<model::ItemId> {
    if (auto source = sourceModel()) {
        auto val = source->data(source->index(idx, 0));
        if (auto pval = get_if<model::ItemId>(&val)) {
            return Some(*pval);
        }
    }
    return None();
}

auto PlayQueue::currentIndex() const -> qint32 { return m_current_index.value(); }
void PlayQueue::setCurrentIndex(qint32 source) { m_current_index = source; }
auto PlayQueue::bindableCurrentIndex() -> const QBindable<qint32> { return &m_current_index; }

auto PlayQueue::currentData(int role) const -> QVariant {
    return data(index(currentIndex(), 0), role);
}

auto PlayQueue::loopMode() const noexcept -> enums::LoopMode { return m_loop_mode; }
void PlayQueue::setLoopMode(enums::LoopMode mode) {
    if (ycore::cmp_set(m_loop_mode, mode)) {
        loopModeChanged(m_loop_mode);
    }
}
void PlayQueue::iterLoopMode() {
    using M   = LoopMode;
    auto mode = loopMode();
    switch (mode) {
    case M::NoneLoop: mode = M::SingleLoop; break;
    case M::SingleLoop: mode = M::ListLoop; break;
    case M::ListLoop: mode = M::ShuffleLoop; break;
    case M::ShuffleLoop: mode = M::NoneLoop; break;
    }
    setLoopMode(mode);
}
auto PlayQueue::randomMode() const noexcept -> bool { return m_random_mode; }
void PlayQueue::setRandomMode(bool v) {
    if (ycore::cmp_set(m_random_mode, v)) {
        randomModeChanged(m_random_mode);
    }
}

auto PlayQueue::canNext() const noexcept -> bool { return m_can_next; }
auto PlayQueue::canPrev() const noexcept -> bool { return m_can_prev; }
auto PlayQueue::canJump() const noexcept -> bool { return m_can_jump; }
auto PlayQueue::canRemove() const noexcept -> bool { return m_can_user_remove; }

void PlayQueue::setCanNext(bool v) {
    if (ycore::cmp_set(m_can_next, v)) {
        canNextChanged();
    }
}
void PlayQueue::setCanPrev(bool v) {
    if (ycore::cmp_set(m_can_prev, v)) {
        canPrevChanged();
    }
}
void PlayQueue::setCanJump(bool v) {
    if (ycore::cmp_set(m_can_jump, v)) {
        canJumpChanged();
    }
}
void PlayQueue::setCanRemove(bool v) {
    if (ycore::cmp_set(m_can_user_remove, v)) {
        canRemoveChanged();
    }
}

void PlayQueue::next() {
    auto mode = loopMode();
    if (mode == LoopMode::SingleLoop) mode = LoopMode::ListLoop;
    next(mode);
}
void PlayQueue::prev() {
    auto mode = loopMode();
    if (mode == LoopMode::SingleLoop) mode = LoopMode::ListLoop;
    prev(mode);
}
void PlayQueue::next(LoopMode mode) {
    bool support_loop = m_options.testFlag(Option::SupportLoop);
    auto count        = m_proxy->rowCount();
    // fire prefetch hint upfront regardless of whether the move succeeds,
    // so dynamic sources can enqueue more rows before we truly run out
    requestNext();
    if (count == 0) {
        if (isDynamic()) schedulePendingAdvance();
        return;
    }
    auto cur = m_proxy->currentIndex();

    switch (mode) {
    case LoopMode::NoneLoop: {
        if (cur + 1 < count) {
            m_proxy->setCurrentIndex(cur + 1);
        } else if (isDynamic()) {
            schedulePendingAdvance();
            return;
        } else {
            return;
        }
        break;
    }
    case LoopMode::ListLoop: {
        m_proxy->setCurrentIndex((cur + 1) % count);
        break;
    }
    case LoopMode::ShuffleLoop: {
        if (m_random_mode) {
            m_proxy->randomIndex();
        } else {
            m_proxy->setCurrentIndex((cur + 1) % count);
        }
        break;
    }
    case LoopMode::SingleLoop: {
    }
    }
    Action::instance()->record(enums::RecordAction::RecordNext);
}
void PlayQueue::prev(LoopMode mode) {
    bool support_loop = m_options.testFlag(Option::SupportLoop);
    auto count        = m_proxy->rowCount();
    if (count == 0) return;
    auto cur = m_proxy->currentIndex();

    switch (mode) {
    case LoopMode::NoneLoop: {
        if (cur >= 1) {
            m_proxy->setCurrentIndex(cur - 1);
        } else {
            return;
        }
        break;
    }
    case LoopMode::ListLoop:
    case LoopMode::ShuffleLoop: {
        m_proxy->setCurrentIndex(cur <= 0 ? std::max(count, 1) - 1 : cur - 1);
        break;
    }
    case LoopMode::SingleLoop: {
    }
    }
    Action::instance()->record(enums::RecordAction::RecordPrev);
}

void PlayQueue::startIfNoCurrent() {
    if (rowCount() > 0 && currentIndex() < 0) {
        m_proxy->setCurrentIndex(0);
        Action::instance()->record(enums::RecordAction::RecordSwitch);
    }
}

void PlayQueue::clear() { removeRows(0, rowCount()); }

auto PlayQueue::update(std::span<const model::Song> in) -> void {
    auto store = AppStore::instance();
    for (auto& el : in) {
        auto item = store->songs.store_item(el.id_proto());
        if (item) {
            m_songs.insert_or_assign(model::item_id(el), *item);
        }
    }
}

void PlayQueue::updateSourceId(std::span<const model::ItemId> songIds,
                               const model::ItemId&           sourceId) {
    for (auto& el : songIds) {
        if (auto it = m_songs.find(el); it != m_songs.end()) {
            // TODO
            // it->second.sourceId = sourceId;
        } else {
            m_source_map.insert_or_assign(el, sourceId);
        }
    }
}

void PlayQueue::onSourceRowsInserted(const QModelIndex&, int first, int last) {
    auto store = AppStore::instance();
    for (int i = first; i <= last; i++) {
        if (auto id = getId(i)) {
            if (m_songs.contains(*id)) continue;
            if (auto song_item = store->songs.store_item(id->id())) {
                m_songs.insert_or_assign(*id, *song_item);
            } else {
                m_pending_ids.insert(*id);
            }
        }
    }
    checkCanMove();
    pendingIdsChanged();

    // if next() previously ran off the end of a dynamic queue, resume advance
    // onto the next proxy row once fresh rows arrive
    if (m_pending_advance) {
        auto proxy_cur   = m_proxy->currentIndex();
        auto proxy_count = m_proxy->rowCount();
        auto target      = proxy_cur + 1;
        if (target >= 0 && target < proxy_count) {
            cancelPendingAdvance();
            m_proxy->setCurrentIndex(target);
        } else if (proxy_cur < 0 && proxy_count > 0) {
            cancelPendingAdvance();
            m_proxy->setCurrentIndex(0);
        }
    }
}

void PlayQueue::onSourceRowsAboutToBeRemoved(const QModelIndex&, int first, int last) {
    for (int i = first; i <= last; i++) {
        auto id = getId(i);
        if (id) {
            m_songs.erase(*id);
            m_source_map.erase(*id);
        }
    }
}
void PlayQueue::onSourceRowsRemoved(const QModelIndex&, int, int) { checkCanMove(); }
void PlayQueue::checkCanMove() {
    auto       count              = rowCount();
    bool       support_prev       = m_options.testFlag(Option::SupportPrev);
    bool       support_loop       = m_options.testFlag(Option::SupportLoop);
    const auto radio              = radioQueue();
    bool       dynamic            = isDynamic() && (! radio || ! radio->exhausted());
    auto       check_on_none_loop = [this, support_prev, count, dynamic] {
        setCanPrev(m_proxy->currentIndex() > 0 && support_prev && count);
        // dynamic sources can always produce more rows, so keep next enabled
        setCanNext((dynamic || count > m_proxy->currentIndex() + 1) && count);
    };
    switch (m_loop_mode) {
    case LoopMode::NoneLoop: {
        check_on_none_loop();
        break;
    }
    case LoopMode::ShuffleLoop:
    case LoopMode::ListLoop: {
        if (support_loop) {
            setCanPrev(support_prev && count);
            setCanNext(count);
        } else {
            check_on_none_loop();
        }
        break;
    }
    default: {
        setCanPrev(support_prev && count);
        setCanNext(count);
    }
    }
}
bool PlayQueue::move(qint32 src, qint32 dst, qint32 count) {
    auto parent = this->index(-1, 0);
    return moveRows(parent, src, count, parent, dst);
}

auto PlayQueue::radioQueue() const -> model::DynamicIdQueue* {
    return qobject_cast<model::DynamicIdQueue*>(sourceModel());
}

auto PlayQueue::dynamicQueue(model::ItemId itemId, const QString& name,
                             const QList<msg::model::QueueConfigField>& configuration,
                             bool can_restart, const QString& restart_label)
    -> model::DynamicIdQueue* {
    auto queueId = itemId.id();
    auto it      = m_dynamic_queues.find(queueId);
    if (it != m_dynamic_queues.end() && it->second) return it->second;
    auto* q =
        new model::DynamicIdQueue(queueId, name, configuration, can_restart, restart_label, this);
    connect(q, &model::DynamicIdQueue::sessionChanged, this, [this, q] {
        if (radioQueue() == q) {
            if (q->exhausted()) cancelPendingAdvance();
            checkCanMove();
        }
    });
    connect(q, &model::DynamicIdQueue::restarting, this, [this, q] {
        if (radioQueue() == q) cancelPendingAdvance();
    });
    connect(q, &model::DynamicIdQueue::restarted, this, [this, q] {
        if (radioQueue() != q) return;
        Action::instance()->record(enums::RecordAction::RecordSwitchQueue);
    });
    m_dynamic_queues[queueId] = q;
    return q;
}

void PlayQueue::addChangedId(std::span<const model::ItemId> ids) {
    m_changed_ids.insert(ids.begin(), ids.end());
    m_changed_timer->start();
}

void PlayQueue::notifyRowsForIds(const std::unordered_set<model::ItemId>& ids) {
    if (ids.empty()) return;
    if (! sourceModel()) return;
    auto count = rowCount();
    for (int row = 0; row < count; row++) {
        auto id = getId(row);
        if (! id || ! ids.contains(*id)) continue;
        auto idx = index(row, 0);
        dataChanged(idx, idx);
        if (row == m_current_index) {
            setCurrentSong(currentIndex());
        }
    }
}

auto PlayQueue::isDynamic() const -> bool { return m_options.testFlag(Option::Dynamic); }

void PlayQueue::schedulePendingAdvance() {
    m_pending_advance = true;
    m_pending_advance_timer->start();
}

void PlayQueue::cancelPendingAdvance() {
    m_pending_advance = false;
    m_pending_advance_timer->stop();
}
void PlayQueue::reloadEntities() {
    for (int row = 0; row < rowCount(); ++row)
        if (auto id = getId(row)) m_pending_ids.insert(*id);
    fetchSongs();
}
void PlayQueue::fetchSongs() {
    if (m_pending_ids.empty() || m_loads.size() >= 8) return;

    auto backend = App::instance()->backend();

    QList<i64> ids;
    for (auto it = m_pending_ids.begin(); it != m_pending_ids.end() && ids.size() < 512;) {
        ids.push_back(it->id());
        it = m_pending_ids.erase(it);
    }
    auto       self   = QPointer<PlayQueue>(this);
    const auto ticket = ++m_loadSerial;
    auto       lease =
        backend->loadSongs(std::move(ids), this, [self, ticket](const Backend::SongResult& rsp) {
            if (! self) return;
            self->m_loads.erase(ticket);
            if (rsp) {
                std::unordered_set<model::ItemId> fetched_ids;
                auto                              store = AppStore::instance();
                for (auto id : rsp->ids()) {
                    model::ItemId key(enums::ItemType::ItemSong, id);
                    if (auto item = store->songs.store_item(id)) {
                        fetched_ids.insert(key);
                        self->m_songs.insert_or_assign(key, *item);
                    }
                }
                self->notifyRowsForIds(fetched_ids);
                self->fetchSongs();
            }
        });
    if (lease) m_loads.emplace(ticket, std::move(lease));
    if (! m_pending_ids.empty() && m_loads.size() < 8) fetchSongs();
}

} // namespace qcm
