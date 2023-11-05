#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "Qcm/model.h"

namespace qcm
{

namespace detail
{
class PlayList;
} // namespace detail

class Playlist : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(model::Song cur READ cur NOTIFY curChanged)
    Q_PROPERTY(qint32 curIndex READ curIndex NOTIFY curIndexChanged)
    Q_PROPERTY(LoopMode loopMode READ loopMode WRITE setLoopMode NOTIFY loopModeChanged)
    Q_PROPERTY(bool canNext READ canNext NOTIFY canMoveChanged)
    Q_PROPERTY(bool canPrev READ canPrev NOTIFY canMoveChanged)
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

signals:
    void curChanged(bool refresh = false);
    void curIndexChanged(bool refresh = false);
    void loopModeChanged();
    void canMoveChanged();
    void end();

public slots:
    void switchList(const std::vector<model::Song>&);
    void switchTo(const model::Song&);
    void next();
    void prev();

    void clear();
    void remove(model::SongId);
    void append(const model::Song&);
    void appendNext(const model::Song&);
    void appendList(const std::vector<model::Song>&);

private slots:
    void setCanNext(bool);
    void setCanPrev(bool);
    void check_cur(bool refresh = false);
    void RefreshCanMove();

private:
    template<typename T>
        requires std::ranges::sized_range<T>
    void insert(int index, const T& range);

    const detail::PlayList& oper_list() const;
    const detail::PlayList& sync_list() const;
    detail::PlayList&       oper_list();
    detail::PlayList&       sync_list();

    model::Song                    m_cur;
    std::map<QString, model::Song> m_songs;
    up<detail::PlayList>           m_list;
    up<detail::PlayList>           m_shuffle_list;
    LoopMode                       m_loop_mode;
    bool                           m_can_next;
    bool                           m_can_prev;
};
} // namespace qcm
