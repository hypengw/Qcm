#include "qcm_interface/model/id_queue.h"

namespace qcm::model
{
IdQueue::IdQueue(QObject* parent): QAbstractListModel(parent), m_current_index(-1) {}
IdQueue::~IdQueue() {}

auto IdQueue::rowCount(const QModelIndex&) const -> int { return m_queue.size(); }
auto IdQueue::data(const QModelIndex& index, int) const -> QVariant {
    if (!index.isValid() || index.row() >= (int)m_queue.size()) return {};
    return QVariant::fromValue(m_queue.at(index.row()));
}

auto IdQueue::currentIndex() const -> qint32 { return m_current_index.value(); }
void IdQueue::setCurrentIndex(qint32 idx) { m_current_index = idx; }
auto IdQueue::bindableCurrentIndex() -> QBindable<qint32> { return &m_current_index; }

void IdQueue::insert(qint32 pos, std::span<const model::ItemId> ids) {
    if (pos > (int)m_queue.size()) return;
    beginInsertRows({}, pos, pos + std::max<qint32>(ids.size(), 1) - 1);
    m_queue.insert(m_queue.begin() + pos, ids.begin(), ids.end());
    endInsertRows();
}
void IdQueue::remove(const model::ItemId& id) {
    if (auto it = std::find_if(m_queue.begin(),
                               m_queue.end(),
                               [&id](const auto& el) {
                                   return el == id;
                               });
        it != m_queue.end()) {
        removeRow(std::distance(m_queue.begin(), it));
    }
}
bool IdQueue::removeRows(int row, int count, const QModelIndex& parent) {
    if (! parent.isValid() || row + count >= (int)m_queue.size()) return false;
    beginRemoveRows(parent, row, row + count);
    m_queue.erase(m_queue.begin() + row);
    endRemoveRows();
    return true;
}
bool IdQueue::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                       const QModelIndex& destinationParent, int destinationChild) {
    return true;
}
} // namespace qcm::model