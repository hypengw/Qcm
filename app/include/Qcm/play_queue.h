#pragma once

#include <QtQml/QQmlEngine>
#include <QtCore/QAbstractListModel>
#include <QtCore/QIdentityProxyModel>
#include <QtCore/QSortFilterProxyModel>

#include "core/core.h"
#include "meta_model/qmetaobjectmodel.h"
#include "qcm_interface/model/query_model.h"
#include "qcm_interface/model/id_queue.h"
#include "qcm_interface/enum.h"
#include "asio_helper/task.h"

namespace qcm
{

class PlayIdQueue : public model::IdQueue {
    Q_OBJECT

public:
    PlayIdQueue(QObject* parent = nullptr);
    ~PlayIdQueue();
};

class PlayIdProxyQueue : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(qint32 currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                   currentIndexChanged BINDABLE bindableCurrentIndex FINAL)
public:
    PlayIdProxyQueue(PlayIdQueue* source, QObject* parent = nullptr);
    ~PlayIdProxyQueue();

    void setSourceModel(QAbstractItemModel* sourceModel) override;
    auto mapToSource(const QModelIndex& proxy_index) const -> QModelIndex override;
    auto mapFromSource(const QModelIndex& source_index) const -> QModelIndex override;
    auto currentIndex() const -> qint32;
    void setCurrentIndex(qint32 idx);
    auto bindableCurrentIndex() -> const QBindable<qint32>;

    Q_SIGNAL void currentIndexChanged(qint32);

private:
    Q_SLOT void onSourceRowsInserted(const QModelIndex& parent, int first, int last);
    Q_SLOT void onSourceRowsRemoved(const QModelIndex& parent, int first, int last);

    auto mapToSource(int row) const -> int;
    auto mapFromSource(int row) const -> int;
    void refreshMap();

    PlayIdQueue*        m_source;
    bool                m_is_shuffle;
    std::vector<qint32> m_shuffle;

    std::unordered_map<qint32, qint32> m_source_to_proxy;
    Q_OBJECT_BINDABLE_PROPERTY(PlayIdProxyQueue, int, m_current_index,
                               &PlayIdProxyQueue::currentIndexChanged)
};

class PlayQueue : public meta_model::QMetaModelBase<QIdentityProxyModel> {
    Q_OBJECT
    QML_UNCREATABLE("")
    Q_PROPERTY(qint32 currentIndex READ currentIndex NOTIFY currentIndexChanged BINDABLE
                   bindableCurrentIndex FINAL)
    Q_PROPERTY(qcm::query::Song currentSong READ currentSong NOTIFY currentSongChanged FINAL)
    Q_PROPERTY(
        qcm::enums::LoopMode loopMode READ loopMode WRITE setLoopMode NOTIFY loopModeChanged FINAL)
    Q_PROPERTY(bool canNext READ canNext NOTIFY canMoveChanged FINAL)
    Q_PROPERTY(bool canPrev READ canPrev NOTIFY canMoveChanged FINAL)

    using base_type = meta_model::QMetaModelBase<QIdentityProxyModel>;

public:
    PlayQueue(QObject* parent = nullptr);
    ~PlayQueue();
    auto data(const QModelIndex& index, int role) const -> QVariant override;

    void setSourceModel(QAbstractItemModel* sourceModel) override;

    auto          currentId() const -> std::optional<model::ItemId>;
    auto          getId(qint32 idx) const -> std::optional<model::ItemId>;
    auto          currentIndex() const -> qint32;
    auto          bindableCurrentIndex() -> const QBindable<qint32>;
    Q_SIGNAL void currentIndexChanged(qint32);

    auto          currentSong() const -> const query::Song&;
    Q_SIGNAL void currentSongChanged();

    auto          loopMode() const -> enums::LoopMode;
    void          setLoopMode(enums::LoopMode mode);
    Q_SIGNAL void loopModeChanged();

    auto          canNext() const -> bool;
    auto          canPrev() const -> bool;
    Q_SIGNAL void canMoveChanged();

    Q_INVOKABLE void clear();

    auto querySongs(std::span<const model::ItemId>) -> task<void>;

private:
    Q_SLOT void onSourceRowsInserted(const QModelIndex& parent, int first, int last);
    Q_SLOT void onSourceRowsRemoved(const QModelIndex& parent, int first, int last);

private:
    query::Song                                    m_empty;
    mutable std::unordered_map<usize, query::Song> m_songs;
    enums::LoopMode                                m_loop_mode;
    Q_OBJECT_BINDABLE_PROPERTY(PlayQueue, int, m_current_index, &PlayQueue::currentIndexChanged)
};

} // namespace qcm
