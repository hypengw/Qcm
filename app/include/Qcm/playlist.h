#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "qcm_interface/model.h"

namespace qcm
{

namespace detail
{
class PlayList;
} // namespace detail

class Playlist : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(model::Song cur READ cur NOTIFY curChanged FINAL)
    Q_PROPERTY(qint32 curIndex READ curIndex NOTIFY curIndexChanged FINAL)
    Q_PROPERTY(LoopMode loopMode READ loopMode WRITE setLoopMode NOTIFY loopModeChanged FINAL)
    Q_PROPERTY(bool canNext READ canNext NOTIFY canMoveChanged FINAL)
    Q_PROPERTY(bool canPrev READ canPrev NOTIFY canMoveChanged FINAL)
public:
    enum LoopMode
    {
        NoneLoop,
        SingleLoop,
        ListLoop,
        ShuffleLoop
    };
    Q_ENUM(LoopMode)

    enum Role
    {
        SongRole,
    };
    Q_ENUM(Role)

    struct CurGuard;

    Playlist(QObject* parent = nullptr);
    ~Playlist();
    std::optional<QString> cur_id() const;

    // prop
    const model::Song& cur() const;
    qint32             curIndex() const;
    LoopMode           loopMode() const;
    void               setLoopMode(LoopMode);
    bool               canNext() const;
    bool               canPrev() const;

    // override
    int      rowCount(const QModelIndex& = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE u32 appendList(const std::vector<model::Song>&);

Q_SIGNALS:
    void curChanged(bool refresh = false);
    void curIndexChanged(bool refresh = false);
    void loopModeChanged();
    void canMoveChanged();
    void end();

public Q_SLOTS:
    void switchList(const std::vector<model::Song>&);
    void switchTo(const model::Song&);
    void next();
    void prev();

    void clear();
    void remove(model::ItemId);
    void append(const model::Song&);
    void appendNext(const model::Song&);
    void iterLoopMode();

private Q_SLOTS:
    void setCanNext(bool);
    void setCanPrev(bool);
    void check_cur(bool refresh = false);
    void RefreshCanMove();

private:
    template<typename T>
        requires std::ranges::sized_range<T> &&
                 std::convertible_to<std::ranges::range_value_t<T>, model::Song>
    usize insert(int index, const T& range);

    const detail::PlayList& oper_list() const;
    const detail::PlayList& sync_list() const;
    detail::PlayList&       oper_list();
    detail::PlayList&       sync_list();

    model::Song                                    m_cur;
    std::unordered_map<model::ItemId, model::Song> m_songs;
    up<detail::PlayList>                           m_list;
    up<detail::PlayList>                           m_shuffle_list;
    LoopMode                                       m_loop_mode;
    bool                                           m_can_next;
    bool                                           m_can_prev;
};
} // namespace qcm
