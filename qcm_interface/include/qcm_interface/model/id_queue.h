#pragma once

#include <unordered_set>
#include <QAbstractListModel>
#include <QObjectBindableProperty>
#include "qcm_interface/item_id.h"

namespace qcm::model
{

class QCM_INTERFACE_API IdQueue : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(qint32 currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                   currentIndexChanged BINDABLE bindableCurrentIndex FINAL)
    Q_PROPERTY(Options options READ options CONSTANT FINAL)
public:
    IdQueue(QObject* parent = nullptr);
    ~IdQueue();

    enum Option
    {
        NoOptions      = 0,
        SupportShuffle = 1,
        SupportLoop    = 1 << 1,
        SupportPrev    = 1 << 2,
    };
    Q_DECLARE_FLAGS(Options, Option)

    auto rowCount(const QModelIndex& parent = QModelIndex()) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;

    auto          options() const -> Options;
    auto          currentIndex() const -> qint32;
    void          setCurrentIndex(qint32 idx);
    void          setCurrentIndex(const model::ItemId&);
    auto          bindableCurrentIndex() -> QBindable<qint32>;
    Q_SIGNAL void currentIndexChanged(qint32 idx);
    Q_SIGNAL void requestNext();

    auto contains(const model::ItemId&) const -> bool;
    void insert(qint32 pos, std::span<const model::ItemId>);
    void remove(const model::ItemId&);
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                  const QModelIndex& destinationParent, int destinationChild) override;

protected:
    void setOptions(Options);

private:
    Q_OBJECT_BINDABLE_PROPERTY(IdQueue, int, m_current_index, &IdQueue::currentIndexChanged)
    std::vector<model::ItemId> m_queue;
    std::unordered_set<usize>  m_set;
    Options                    m_opts;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(IdQueue::Options)

} // namespace qcm::model