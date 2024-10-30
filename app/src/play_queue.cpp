#include "Qcm/play_queue.h"

#include <ranges>
#include <variant>

#include "core/random.h"
#include "core/log.h"
#include "core/optional_helper.h"
#include "asio_helper/basic.h"
#include "qcm_interface/global.h"

namespace qcm
{

namespace
{
auto get_hash(const model::ItemId& id) -> usize { return std::hash<model::ItemId> {}(id); }
} // namespace

PlayIdQueue::PlayIdQueue(QObject* parent): model::IdQueue(parent) {}
PlayIdQueue::~PlayIdQueue() {}

PlayIdProxyQueue::PlayIdProxyQueue(PlayIdQueue* source, QObject* parent)
    : QSortFilterProxyModel(parent), m_source(source), m_current_index(-1) {
    _assert_rel_(source);
    QSortFilterProxyModel::setSourceModel(source);

    auto source_idx = source->bindableCurrentIndex();

    m_current_index.setBinding([source_idx, this] {
        return mapFromSource(source_idx.value());
    });

    refreshMap();
}
PlayIdProxyQueue::~PlayIdProxyQueue() {}
void PlayIdProxyQueue::setSourceModel(QAbstractItemModel*) {}
auto PlayIdProxyQueue::currentIndex() const -> qint32 { return m_current_index.value(); }
void PlayIdProxyQueue::setCurrentIndex(qint32 idx) { m_source->setCurrentIndex(mapToSource(idx)); }
auto PlayIdProxyQueue::bindableCurrentIndex() -> const QBindable<qint32> {
    return &m_current_index;
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
    if (row >= (int)m_shuffle.size()) return -1;
    return m_is_shuffle ? m_shuffle[row] : row;
}

auto PlayIdProxyQueue::mapFromSource(int row) const -> int {
    int proxy_row { -1 };
    if (m_is_shuffle) {
        proxy_row = helper::to_optional(m_source_to_proxy, row).value_or(-1);
    } else {
        proxy_row = row;
    }
    return proxy_row;
}

void PlayIdProxyQueue::refreshMap() {
    int rowCount = sourceModel()->rowCount();
    m_shuffle.resize(rowCount);
    m_source_to_proxy.clear();

    // Fill with sequential indices and shuffle them
    std::iota(m_shuffle.begin(), m_shuffle.end(), 0);
    Random::shuffle(m_shuffle.begin(), m_shuffle.end());

    // Populate the reverse mapping
    for (int proxyRow = 0; proxyRow < rowCount; ++proxyRow) {
        int sourceRow                = m_shuffle[proxyRow];
        m_source_to_proxy[sourceRow] = proxyRow;
    }
}
void PlayIdProxyQueue::onSourceRowsInserted(const QModelIndex& parent, int first, int last) {
    refreshMap();
}
void PlayIdProxyQueue::onSourceRowsRemoved(const QModelIndex& parent, int first, int last) {
    refreshMap();
}

PlayQueue::PlayQueue(QObject* parent): meta_model::QMetaModelBase<QIdentityProxyModel>(parent) {
    updateRoleNames(query::Song::staticMetaObject);
}
PlayQueue::~PlayQueue() {}

auto PlayQueue::data(const QModelIndex& index, int role) const -> QVariant {
    auto row = index.row();
    auto id  = getId(row);
    do {
        if (! id) break;
        auto hash = std::hash<model::ItemId> {}(id.value());
        auto it   = m_songs.find(hash);
        if (it == m_songs.end()) {
            query::Song song;
            song.id = id.value();
            it      = m_songs.insert({ hash, song }).first;
        }

        if (auto prop = this->propertyOfRole(role); prop) {
            return prop.value().readOnGadget(&*it);
        }
    } while (0);
    return {};
}

void PlayQueue::setSourceModel(QAbstractItemModel* sourceModel) {
    base_type::setSourceModel(sourceModel);
    QBindable<qint32> source_idx(sourceModel, "currentIndex");
    m_current_index.setBinding(source_idx.binding());
}

auto PlayQueue::currentSong() const -> const query::Song& { return m_empty; }

auto PlayQueue::currentId() const -> std::optional<model::ItemId> { return getId(currentIndex()); }

auto PlayQueue::getId(qint32 idx) const -> std::optional<model::ItemId> {
    if (auto source = sourceModel()) {
        auto val = source->data(source->index(idx, 0));
        if (auto pval = get_if<model::ItemId>(&val)) {
            return *pval;
        }
    }
    return std::nullopt;
}

auto PlayQueue::currentIndex() const -> qint32 { return m_current_index.value(); }
auto PlayQueue::bindableCurrentIndex() -> const QBindable<qint32> { return &m_current_index; }

auto PlayQueue::loopMode() const -> enums::LoopMode { return m_loop_mode; }
void PlayQueue::setLoopMode(enums::LoopMode mode) {
    if (ycore::cmp_exchange(m_loop_mode, mode)) {
        loopModeChanged();
    }
}
auto PlayQueue::canNext() const -> bool { return true; }
auto PlayQueue::canPrev() const -> bool { return true; }

void PlayQueue::clear() {}

auto PlayQueue::querySongs(std::span<const model::ItemId> ids) -> task<void> { 

    co_return; 
}

void PlayQueue::onSourceRowsInserted(const QModelIndex&, int first, int last) {
    std::vector<model::ItemId> ids;
    for (int i = first; i <= last; i++) {
        if (auto id = getId(i)) {
            ids.emplace_back(id.value());
        }
    }

    auto ex = asio::make_strand(Global::instance()->pool_executor());
    asio::co_spawn(
        ex,
        [ids, this] -> task<void> {
            co_await querySongs(ids);
            co_return;
        },
        helper::asio_detached_log_t {});
}
void PlayQueue::onSourceRowsRemoved(const QModelIndex&, int first, int last) {
    for (int i = first; i <= last; i++) {
        auto id = getId(i);
        if (id) {
            m_songs.erase(get_hash(*id));
        }
    }
}

} // namespace qcm