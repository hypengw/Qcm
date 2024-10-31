#include "qcm_interface/model/id_queue.h"

#include <ranges>
namespace qcm::model
{
IdQueue::IdQueue(QObject* parent): QAbstractListModel(parent), m_current_index(-1) {}
IdQueue::~IdQueue() {}

auto IdQueue::rowCount(const QModelIndex&) const -> int { return m_queue.size(); }
auto IdQueue::data(const QModelIndex& index, int) const -> QVariant {
    if (! index.isValid() || index.row() >= (int)m_queue.size()) return {};
    return QVariant::fromValue(m_queue.at(index.row()));
}

auto IdQueue::options() const -> Options { return m_opts; }
void IdQueue::setOptions(Options opts) { m_opts = opts; }
auto IdQueue::currentIndex() const -> qint32 { return m_current_index.value(); }
void IdQueue::setCurrentIndex(qint32 idx) { m_current_index = idx; }
auto IdQueue::bindableCurrentIndex() -> QBindable<qint32> { return &m_current_index; }
void IdQueue::setCurrentIndex(const model::ItemId& id) {
    qint32 i = 0;
    for (auto& el : m_queue) {
        if (el == id) {
            setCurrentIndex(i);
        }
        i++;
    }
}

auto IdQueue::contains(const model::ItemId& id) const -> bool {
    auto hash = std::hash<model::ItemId> {}(id);
    return m_set.contains(hash);
}

void IdQueue::insert(qint32 pos, std::span<const model::ItemId> ids) {
    if (pos > (int)m_queue.size()) return;
    std::vector<usize>         hashes;
    std::vector<model::ItemId> insert_ids;
    for (auto& el : ids) {
        auto hash = std::hash<model::ItemId>()(el);
        if (! m_set.contains(hash)) {
            hashes.push_back(hash);
            insert_ids.emplace_back(el);
        }
    }

    if (hashes.empty()) return;
    beginInsertRows({}, pos, pos + hashes.size() - 1);

    m_queue.insert(m_queue.begin() + pos, insert_ids.begin(), insert_ids.end());
    m_set.insert(hashes.begin(), hashes.end());

    endInsertRows();

    if (m_current_index == -1) {
        setCurrentIndex(0);
    }
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
bool IdQueue::removeRows(int first, int count, const QModelIndex& parent) {
    if (count <= 0) return false;
    count     = std::max(count, 1) - 1;
    auto last = std::min(first + count, std::max<int>(m_queue.size(), 1) - 1);

    beginRemoveRows(parent, first, last);
    auto begin = m_queue.begin() + first;
    auto end   = begin + count + 1;
    {
        auto it = begin;
        while (it != end) m_set.erase(std::hash<model::ItemId> {}(*(it++)));
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
    return true;
}
} // namespace qcm::model