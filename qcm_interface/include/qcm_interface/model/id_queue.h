#pragma once

#include <QAbstractListModel>
#include <QObjectBindableProperty>
#include "qcm_interface/item_id.h"

namespace qcm::model
{

class QCM_INTERFACE_API IdQueue : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(qint32 currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                   currentIndexChanged BINDABLE bindableCurrentIndex FINAL)

public:
    IdQueue(QObject* parent = nullptr);
    ~IdQueue();

    auto rowCount(const QModelIndex& parent = QModelIndex()) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;

    auto          currentIndex() const -> qint32;
    void          setCurrentIndex(qint32 idx);
    auto          bindableCurrentIndex() -> QBindable<qint32>;
    Q_SIGNAL void currentIndexChanged(qint32 idx);

    void insert(qint32 pos, std::span<const model::ItemId>);
    void remove(const model::ItemId&);
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                  const QModelIndex& destinationParent, int destinationChild) override;

private:
    Q_OBJECT_BINDABLE_PROPERTY(IdQueue, int, m_current_index, &IdQueue::currentIndexChanged)
    std::vector<model::ItemId> m_queue;
};

} // namespace qcm::model