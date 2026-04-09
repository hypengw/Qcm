module;
#include "Qcm/model/id_queue.moc.h"
module qcm;
import :model.id_queue;
import :global;
import :app;
import :query.play;

namespace qcm::model
{
IdQueue::IdQueue(QObject* parent)
    : QAbstractListModel(parent), m_current_index(-1), m_name("Queue") {
    connect(this, &IdQueue::currentIndexChanged, this, &IdQueue::currentIdChanged);
}
IdQueue::~IdQueue() {}

auto IdQueue::rowCount(const QModelIndex&) const -> int { return m_queue.size(); }
auto IdQueue::data(const QModelIndex& index, int) const -> QVariant {
    if (! index.isValid() || index.row() >= (int)m_queue.size()) return {};
    return QVariant::fromValue(m_queue.at(index.row()).id);
}
auto IdQueue::name() const -> const QString& { return m_name; }
void IdQueue::setName(QStringView name) { m_name = name.toString(); }

auto IdQueue::options() const -> Options { return m_opts; }
void IdQueue::setOptions(Options opts) { m_opts = opts; }
auto IdQueue::currentId() const -> model::ItemId {
    auto cur = currentIndex();
    if (cur < 0 || cur >= rowCount()) {
        return {};
    }
    return m_queue.at(cur).id;
}

auto IdQueue::currentExtra() const -> std::optional<QVariant> {
    auto cur = currentIndex();
    if (cur < 0 || cur > rowCount()) {
        return std::nullopt;
    }
    return m_queue.at(cur).extra;
}

auto IdQueue::currentOrFirstExtra() const -> std::optional<QVariant> {
    auto cur = currentIndex();
    if (cur < 0 || cur > rowCount()) {
        if (rowCount() > 0) {
            cur = 0;
        } else {
            return std::nullopt;
        }
    }
    return m_queue.at(cur).extra;
}

auto IdQueue::currentIndex() const -> qint32 { return m_current_index.value(); }
void IdQueue::setCurrentIndex(qint32 idx) { m_current_index = idx; }
auto IdQueue::bindableCurrentIndex() -> QBindable<qint32> { return &m_current_index; }
void IdQueue::setCurrentIndex(const model::ItemId& id) {
    qint32 i = 0;
    for (auto& el : m_queue) {
        if (el.id == id) {
            setCurrentIndex(i);
            break;
        }
        i++;
    }
}

auto IdQueue::contains(const model::ItemId& id) const -> bool {
    auto hash = std::hash<model::ItemId> {}(id);
    return m_set.contains(hash);
}

auto IdQueue::insert(qint32 pos, std::span<const model::ItemId> ids) -> int {
    if (pos > (int)m_queue.size()) return 0;
    std::vector<usize> hashes;
    std::vector<Item>  insert_ids;
    for (auto& el : ids) {
        auto hash = std::hash<model::ItemId>()(el);
        if (! m_set.contains(hash)) {
            hashes.push_back(hash);
            insert_ids.emplace_back(el);
        }
    }

    if (hashes.empty()) return 0;
    beginInsertRows({}, pos, pos + hashes.size() - 1);

    m_queue.insert(m_queue.begin() + pos, insert_ids.begin(), insert_ids.end());
    m_set.insert(hashes.begin(), hashes.end());

    endInsertRows();
    return hashes.size();
}

auto IdQueue::insert(qint32 pos, std::span<const Item> items) -> int {
    if (pos > (int)m_queue.size()) return 0;
    std::vector<usize> hashes;
    std::vector<Item>  insert_items;
    for (auto& el : items) {
        auto hash = std::hash<model::ItemId>()(el.id);
        if (! m_set.contains(hash)) {
            hashes.push_back(hash);
            insert_items.emplace_back(el);
        }
    }

    if (hashes.empty()) return 0;
    beginInsertRows({}, pos, pos + hashes.size() - 1);

    m_queue.insert(m_queue.begin() + pos, insert_items.begin(), insert_items.end());
    m_set.insert(hashes.begin(), hashes.end());

    endInsertRows();
    return hashes.size();
}

void IdQueue::remove(const model::ItemId& id) {
    if (auto it = std::find_if(m_queue.begin(),
                               m_queue.end(),
                               [&id](const auto& el) {
                                   return el.id == id;
                               });
        it != m_queue.end()) {
        removeRow(std::distance(m_queue.begin(), it));
    }
}
bool IdQueue::removeRows(int first, int count, const QModelIndex& parent) {
    if (count <= 0) return false;
    count     = std::max(count, 1) - 1;
    auto last = std::min(first + count, std::max<int>(m_queue.size(), 1) - 1);

    beginRemoveRows(parent, first, last);
    auto begin = m_queue.begin() + first;
    auto end   = begin + count + 1;
    {
        auto it = begin;
        while (it != end) m_set.erase(std::hash<model::ItemId> {}((it++)->id));
    }
    m_queue.erase(begin, end);
    endRemoveRows();

    if (m_current_index > last) {
        setCurrentIndex(m_current_index - (count + 1));
    } else if (m_current_index <= last && m_current_index >= first) {
        setCurrentIndex(-1);
    } else {
    }
    return true;
}
bool IdQueue::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                       const QModelIndex& destinationParent, int destinationChild) {
    if (sourceRow < 0 || sourceRow + count - 1 >= rowCount(sourceParent) || destinationChild < 0 ||
        destinationChild > rowCount(destinationParent) || sourceRow == destinationChild - 1 ||
        count <= 0 || sourceParent.isValid() || destinationParent.isValid()) {
        return false;
    }
    if (! beginMoveRows(
            QModelIndex(), sourceRow, sourceRow + count - 1, QModelIndex(), destinationChild))
        return false;
    {
        auto it  = m_queue.begin();
        auto src = it + sourceRow;
        auto dst = it + destinationChild;
        if (src > dst) {
            std::rotate(dst, src, src + count);
        } else {
            std::rotate(src, src + count, dst);
        }
    }
    endMoveRows();
    return true;
}

bool IdQueue::move(qint32 src, qint32 dst, qint32 count) {
    auto p = index(-1);
    return moveRows(p, src, count, p, dst);
}

DynamicIdQueue::DynamicIdQueue(qint64 queue_id, QObject* parent)
    : IdQueue(parent),
      m_queue_id(queue_id),
      m_query(new QueueNextQuery(this)),
      m_last_idx(-1),
      m_auto_removing(false) {
    setName(QString::number(queue_id));
    setOptions(SupportPrev | Dynamic);

    auto query = static_cast<QueueNextQuery*>(m_query);
    query->setQueueId(m_queue_id);
    connect(query, &QueueNextQuery::statusChanged, this, &DynamicIdQueue::on_query_finished);

    connect(this, &IdQueue::requestNext, this, &DynamicIdQueue::on_request_next);
    connect(this,
            &IdQueue::currentIndexChanged,
            this,
            &DynamicIdQueue::on_current_index_changed);

    QMetaObject::invokeMethod(this, &DynamicIdQueue::on_request_next, Qt::QueuedConnection);
}

auto DynamicIdQueue::queueId() const -> qint64 { return m_queue_id; }

void DynamicIdQueue::on_current_index_changed(qint32 idx) {
    if (m_auto_removing) return;
    on_request_next();
    // only auto-drop history when moving forward
    if (idx > m_last_idx && idx >= 2) {
        m_auto_removing = true;
        removeRows(idx - 2, 1);
        m_auto_removing = false;
        // after removing row (idx-2), current shifts down by 1
        m_last_idx = idx - 1;
    } else {
        m_last_idx = idx;
    }
}

void DynamicIdQueue::on_request_next() {
    constexpr int k_prefetch_threshold = 3;
    auto query = static_cast<QueueNextQuery*>(m_query);
    if (query->querying()) return;
    // skip when there are still enough rows ahead of the current item
    if (currentIndex() < rowCount() - k_prefetch_threshold) return;
    query->reload();
}

void DynamicIdQueue::on_query_finished() {
    auto query = static_cast<QueueNextQuery*>(m_query);
    if (query->status() != QAsyncResult::Status::Finished) return;
    const bool was_empty = rowCount() == 0;
    std::vector<model::ItemId> ids;
    for (auto& s : query->tdata()) {
        ids.push_back(model::ItemId(enums::ItemType::ItemSong, s.id_proto()));
    }
    insert(rowCount(), ids);
    // expose a current id on the first batch so cards can show a cover even
    // before playback ever starts; guard auto_removing so the resulting
    // on_current_index_changed won't trigger a chained prefetch
    if (was_empty && currentIndex() < 0 && rowCount() > 0) {
        m_auto_removing = true;
        setCurrentIndex(0);
        m_auto_removing = false;
        m_last_idx = 0;
    }
}

} // namespace qcm::model
