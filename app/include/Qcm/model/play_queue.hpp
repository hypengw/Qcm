#pragma once

#include <QtQml/QQmlEngine>
#include <QtCore/QAbstractListModel>
#include <QtCore/QIdentityProxyModel>
#include <QtCore/QSortFilterProxyModel>

#include "core/core.h"
#include "kstore/qt/meta_list_model.hpp"
#include "Qcm/model/id_queue.hpp"
#include "Qcm/qml/enum.hpp"
#include "Qcm/store.hpp"

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
    auto bindableCurrentIndex() -> const QBindable<qint32>;
    auto randomIndex() -> int;

    Q_SLOT void   setCurrentIndex(qint32 idx);
    Q_SIGNAL void currentIndexChanged(qint32);

private:
    Q_SLOT void   setCurrentIndexFromSource(qint32 idx);
    auto        useShuffle() const -> bool;
    Q_SLOT void onSourceRowsAboutToBeInserted(const QModelIndex& parent, int first, int last);
    Q_SLOT void onSourceRowsInserted(const QModelIndex& parent, int first, int last);
    Q_SLOT void onSourceRowsRemoved(const QModelIndex& parent, int first, int last);
    Q_SLOT void onSourceRowsMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
                                  const QModelIndex& destinationParent, int destinationRow);

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

class PlayQueue : public QIdentityProxyModel, public kstore::QMetaRoleNames {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(qint32 currentIndex READ currentIndex NOTIFY currentIndexChanged BINDABLE
                   bindableCurrentIndex FINAL)
    Q_PROPERTY(qcm::model::Song currentSong READ currentSong NOTIFY currentSongChanged FINAL)
    Q_PROPERTY(
        qcm::enums::LoopMode loopMode READ loopMode WRITE setLoopMode NOTIFY loopModeChanged FINAL)
    Q_PROPERTY(bool randomMode READ randomMode WRITE setRandomMode NOTIFY randomModeChanged FINAL)
    Q_PROPERTY(bool canNext READ canNext NOTIFY canNextChanged FINAL)
    Q_PROPERTY(bool canPrev READ canPrev NOTIFY canPrevChanged FINAL)
    Q_PROPERTY(bool canJump READ canJump NOTIFY canJumpChanged FINAL)
    Q_PROPERTY(bool canRemove READ canRemove NOTIFY canRemoveChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged FINAL)

public:
    using LoopMode = enums::LoopMode;
    using Option   = model::IdQueue::Option;
    using Song     = model::Song;
    using SongItem = AppStore::song_item;

    PlayQueue(QObject* parent = nullptr);
    ~PlayQueue();

    auto data(const QModelIndex& index, int role) const -> QVariant override;
    void setSourceModel(QAbstractItemModel* sourceModel) override;

    auto          name() const -> const QString&;
    auto          currentId() const -> rstd::Option<model::ItemId>;
    auto          getId(qint32 idx) const -> rstd::Option<model::ItemId>;
    auto          currentIndex() const -> qint32;
    auto          bindableCurrentIndex() -> const QBindable<qint32>;
    Q_SLOT void   setCurrentIndex(qint32);
    Q_SIGNAL void currentIndexChanged(qint32);
    auto          currentData(int role) const -> QVariant;

    auto          currentSong() const -> Song;
    void          setCurrentSong(rstd::Option<SongItem>);
    Q_SLOT void   setCurrentSong(qint32 idx);
    Q_SIGNAL void currentSongChanged();

    auto          loopMode() const -> enums::LoopMode;
    void          setLoopMode(enums::LoopMode mode);
    Q_SLOT void   iterLoopMode();
    Q_SIGNAL void loopModeChanged(enums::LoopMode);

    auto          randomMode() const -> bool;
    void          setRandomMode(bool);
    Q_SIGNAL void randomModeChanged(bool);

    auto          canNext() const -> bool;
    auto          canPrev() const -> bool;
    auto          canJump() const -> bool;
    auto          canRemove() const -> bool;
    void          setCanNext(bool);
    void          setCanPrev(bool);
    void          setCanJump(bool);
    void          setCanRemove(bool);
    Q_SIGNAL void canNextChanged();
    Q_SIGNAL void canPrevChanged();
    Q_SIGNAL void canJumpChanged();
    Q_SIGNAL void canRemoveChanged();
    Q_SIGNAL void nameChanged();

    Q_SLOT void next();
    Q_SLOT void prev();
    Q_SLOT void next(LoopMode mode);
    Q_SLOT void prev(LoopMode mode);
    Q_SLOT void startIfNoCurrent();

    Q_SLOT void   clear();
    Q_SIGNAL void requestNext();

    Q_INVOKABLE bool move(qint32 src, qint32 dst, qint32 count = 1);

    auto update(std::span<const model::Song>) -> void;
    void updateSourceId(std::span<const model::ItemId> songIds, const model::ItemId& sourceId);

private:
    Q_SLOT void onSourceRowsInserted(const QModelIndex& parent, int first, int last);
    Q_SLOT void onSourceRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
    Q_SLOT void onSourceRowsRemoved(const QModelIndex& parent, int first, int last);
    Q_SLOT void checkCanMove();

private:
    PlayIdProxyQueue*       m_proxy;
    rstd::Option<SongItem>  m_current_song;
    Song                    m_placeholder;
    enums::LoopMode         m_loop_mode;
    model::IdQueue::Options m_options;

    mutable std::unordered_map<model::ItemId, SongItem>      m_songs;
    mutable std::unordered_map<model::ItemId, model::ItemId> m_source_map;

    bool    m_can_next;
    bool    m_can_prev;
    bool    m_can_jump;
    bool    m_can_user_remove;
    bool    m_random_mode;
    QString m_name;

    Q_OBJECT_BINDABLE_PROPERTY(PlayQueue, int, m_current_index, &PlayQueue::currentIndexChanged)
};

} // namespace qcm
