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

class PlayIdProxyQueue : public QIdentityProxyModel {
    Q_OBJECT
    Q_PROPERTY(qint32 currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                   currentIndexChanged BINDABLE bindableCurrentIndex FINAL)
    Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged BINDABLE
                   bindableShuffle FINAL)
public:
    PlayIdProxyQueue(QObject* parent = nullptr);
    ~PlayIdProxyQueue();

    void setSourceModel(QAbstractItemModel* sourceModel) override;
    auto mapToSource(const QModelIndex& proxy_index) const -> QModelIndex override;
    auto mapFromSource(const QModelIndex& source_index) const -> QModelIndex override;

    auto          shuffle() const -> bool;
    void          setShuffle(bool);
    auto          bindableShuffle() -> QBindable<bool>;
    Q_SIGNAL void shuffleChanged();

    auto currentIndex() const -> qint32;
    void setCurrentIndex(qint32 idx);
    auto bindableCurrentIndex() -> const QBindable<qint32>;

    Q_SIGNAL void currentIndexChanged(qint32);

private:
    auto        useShuffle() const -> bool;
    Q_SLOT void onSourceRowsAboutToBeInserted(const QModelIndex& parent, int first, int last);
    Q_SLOT void onSourceRowsInserted(const QModelIndex& parent, int first, int last);
    Q_SLOT void onSourceRowsRemoved(const QModelIndex& parent, int first, int last);

    auto mapToSource(int row) const -> int;
    auto mapFromSource(int row) const -> int;

    Q_SLOT void reShuffle();
    void        shuffleSync();
    void        refreshFromSource();

    PlayIdQueue*        m_source;
    QProperty<bool>     m_support_shuffle;
    std::vector<qint32> m_shuffle_list;

    std::unordered_map<qint32, qint32> m_source_to_proxy;
    Q_OBJECT_BINDABLE_PROPERTY(PlayIdProxyQueue, int, m_current_index,
                               &PlayIdProxyQueue::currentIndexChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PlayIdProxyQueue, bool, m_shuffle, &PlayIdProxyQueue::shuffleChanged)
};

class PlayQueue : public meta_model::QMetaModelBase<QIdentityProxyModel> {
    Q_OBJECT
    QML_UNCREATABLE("")
    Q_PROPERTY(qint32 currentIndex READ currentIndex NOTIFY currentIndexChanged BINDABLE
                   bindableCurrentIndex FINAL)
    Q_PROPERTY(qcm::query::Song currentSong READ currentSong NOTIFY currentSongChanged FINAL)
    Q_PROPERTY(
        qcm::enums::LoopMode loopMode READ loopMode WRITE setLoopMode NOTIFY loopModeChanged FINAL)
    Q_PROPERTY(bool canNext READ canNext NOTIFY canNextChanged FINAL)
    Q_PROPERTY(bool canPrev READ canPrev NOTIFY canPrevChanged FINAL)

    using base_type = meta_model::QMetaModelBase<QIdentityProxyModel>;

public:
    using LoopMode = enums::LoopMode;
    using Option   = model::IdQueue::Option;

    PlayQueue(QObject* parent = nullptr);
    ~PlayQueue();
    auto data(const QModelIndex& index, int role) const -> QVariant override;

    void setSourceModel(QAbstractItemModel* sourceModel) override;

    auto          currentId() const -> std::optional<model::ItemId>;
    auto          getId(qint32 idx) const -> std::optional<model::ItemId>;
    auto          currentIndex() const -> qint32;
    auto          bindableCurrentIndex() -> const QBindable<qint32>;
    Q_SIGNAL void currentIndexChanged(qint32);

    auto          currentSong() const -> query::Song;
    void          setCurrentSong(const std::optional<query::Song>&);
    Q_SLOT void   setCurrentSong(qint32 idx);
    Q_SIGNAL void currentSongChanged();

    auto          loopMode() const -> enums::LoopMode;
    void          setLoopMode(enums::LoopMode mode);
    Q_SLOT void   iterLoopMode();
    Q_SIGNAL void loopModeChanged();

    auto          canNext() const -> bool;
    auto          canPrev() const -> bool;
    void          setCanNext(bool);
    void          setCanPrev(bool);
    Q_SIGNAL void canNextChanged();
    Q_SIGNAL void canPrevChanged();

    Q_SLOT void next();
    Q_SLOT void prev();
    Q_SLOT void next(LoopMode mode);
    Q_SLOT void prev(LoopMode mode);

    Q_INVOKABLE void clear();

    auto querySongsSql(std::span<const model::ItemId>) -> task<std::vector<query::Song>>;
    auto querySongs(std::span<const model::ItemId>) -> task<void>;

private:
    Q_SIGNAL void requestNext();
    Q_SLOT void   onSourceRowsInserted(const QModelIndex& parent, int first, int last);
    Q_SLOT void   onSourceRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
    Q_SLOT void   onSourceRowsRemoved(const QModelIndex& parent, int first, int last);
    Q_SLOT void   checkCanMove();

private:
    PlayIdProxyQueue*                              m_proxy;
    std::optional<query::Song>                     m_current_song;
    query::Song                                    m_placeholder;
    mutable std::unordered_map<usize, query::Song> m_songs;
    enums::LoopMode                                m_loop_mode;
    model::IdQueue::Options                        m_options;

    bool m_can_next;
    bool m_can_prev;

    Q_OBJECT_BINDABLE_PROPERTY(PlayQueue, int, m_current_index, &PlayQueue::currentIndexChanged)
};

} // namespace qcm
