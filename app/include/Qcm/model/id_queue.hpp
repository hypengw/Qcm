#pragma once

#include <unordered_set>
#include <QAbstractListModel>
#include <QObjectBindableProperty>
#include "Qcm/model/item_id.hpp"

namespace qcm::model
{

class IdQueue : public QAbstractListModel {
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
        model::ItemId id;
        QVariant      extra;
    };

    auto rowCount(const QModelIndex& parent = QModelIndex()) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;

    auto          options() const -> Options;
    auto          currentId() const -> std::optional<model::ItemId>;
    auto          currentExtra() const -> std::optional<QVariant>;
    auto          currentOrFirstExtra() const -> std::optional<QVariant>;
    auto          currentIndex() const -> qint32;
    void          setCurrentIndex(qint32 idx);
    void          setCurrentIndex(const model::ItemId&);
    auto          bindableCurrentIndex() -> QBindable<qint32>;
    Q_SIGNAL void currentIndexChanged(qint32 idx);
    Q_SIGNAL void requestNext();
    auto          contains(const model::ItemId&) const -> bool;
    auto          insert(qint32 pos, std::span<const model::ItemId>) -> int;
    auto          insert(qint32 pos, std::span<const Item>) -> int;
    void          remove(const model::ItemId&);
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool             moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
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