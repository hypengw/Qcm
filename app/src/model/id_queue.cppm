module;

#include <unordered_set>
#include <ranges>
#include <QAbstractListModel>
#include <QObjectBindableProperty>
#include "Qcm/model/id_queue.moc.h"

#ifdef Q_MOC_RUN
#include "Qcm/model/id_queue.moc"
#endif

export module qcm.model.id_queue;
export import qcm.model.item_id;

namespace qcm::model
{

export class IdQueue : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(qint32 currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                   currentIndexChanged BINDABLE bindableCurrentIndex FINAL)
    Q_PROPERTY(Options options READ options CONSTANT FINAL)
    Q_PROPERTY(QString name READ name CONSTANT FINAL)
public:
    IdQueue(QObject* parent = nullptr);
    ~IdQueue();

    enum Option
    {
        NoOptions         = 0,
        SupportShuffle    = 1,
        SupportLoop       = 1 << 1,
        SupportPrev       = 1 << 2,
        SupportJump       = 1 << 3,
        SupportUserRemove = 1 << 4,
    };
    Q_DECLARE_FLAGS(Options, Option)

    struct Item {
        ItemId   id;
        QVariant extra;
    };

    auto rowCount(const QModelIndex& parent = QModelIndex()) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;

    auto          options() const -> Options;
    auto          currentId() const -> std::optional<ItemId>;
    auto          currentExtra() const -> std::optional<QVariant>;
    auto          currentOrFirstExtra() const -> std::optional<QVariant>;
    auto          currentIndex() const -> qint32;
    Q_SLOT void   setCurrentIndex(qint32 idx);
    void          setCurrentIndex(const ItemId&);
    auto          bindableCurrentIndex() -> QBindable<qint32>;
    Q_SIGNAL void currentIndexChanged(qint32 idx);
    Q_SIGNAL void requestNext();
    auto          contains(const ItemId&) const -> bool;
    auto          insert(qint32 pos, std::span<const ItemId>) -> int;
    auto          insert(qint32 pos, std::span<const Item>) -> int;
    void          remove(const ItemId&);
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                  const QModelIndex& destinationParent, int destinationChild) override;

    Q_INVOKABLE bool move(qint32 src, qint32 dst, qint32 count = 1);

    auto name() const -> const QString&;

protected:
    void setOptions(Options);
    void setName(QStringView);

private:
    Q_OBJECT_BINDABLE_PROPERTY(IdQueue, int, m_current_index, &IdQueue::currentIndexChanged)
    std::vector<Item>         m_queue;
    std::unordered_set<usize> m_set;
    Options                   m_opts;
    QString                   m_name;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(IdQueue::Options)

} // namespace qcm::model

module :private;


namespace qcm::model
{
IdQueue::IdQueue(QObject* parent)
    : QAbstractListModel(parent), m_current_index(-1), m_name("Queue") {}
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
auto IdQueue::currentId() const -> std::optional<model::ItemId> {
    auto cur = currentIndex();
    if (cur < 0 || cur > rowCount()) {
        return std::nullopt;
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
} // namespace qcm::model

#include "Qcm/model/id_queue.moc.cpp"