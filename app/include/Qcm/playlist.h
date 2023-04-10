#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "Qcm/model.h"

namespace qcm
{

namespace detail
{
class PlayList {
public:
    using list_type      = std::vector<QString>;
    using iterator       = list_type::iterator;
    using const_iterator = list_type::const_iterator;

    auto begin() const { return m_list.begin(); }
    auto end() const { return m_list.end(); }
    auto size() const { return m_list.size(); }
    auto data() const { return m_list.data(); }
    auto begin() { return m_list.begin(); }
    auto end() { return m_list.end(); }
    auto pos_it(const QString& s) const {
        return std::find_if(begin(), end(), [&s](auto& s_) {
            return s == s_;
        });
    }
    std::optional<usize> pos(const QString& s) const {
        auto it = pos_it(s);
        return it == end() ? std::nullopt : std::make_optional((usize)std::distance(begin(), it));
    }

    bool                   cur_ok() const { return (bool)m_cur_pos; }
    auto                   cur_it() const { return cur_ok() ? begin() + m_cur_pos.value() : end(); }
    auto                   cur_it() { return cur_ok() ? begin() + m_cur_pos.value() : end(); }
    std::optional<usize>   cur_pos() const { return m_cur_pos; }
    std::optional<QString> cur() const {
        return m_cur_pos ? std::make_optional(m_list.at(m_cur_pos.value())) : std::nullopt;
    }
    auto erase(const_iterator it) {
        const auto& self = *this;
        auto        d    = (usize)std::distance(self.begin(), it);
        if (m_cur_pos) {
            auto p = m_cur_pos.value();
            if (d < p)
                m_cur_pos = p - 1;
            else if (d == p)
                m_cur_pos = std::nullopt;
        }

        return m_list.erase(it);
    }
    auto insert(const_iterator it, const QString& s) {
        const auto& self = *this;
        auto        d    = (usize)std::distance(self.begin(), it);
        if (m_cur_pos) {
            auto p = m_cur_pos.value();
            if (d <= p) m_cur_pos = p + 1;
        }
        return m_list.insert(it, s);
    }
    auto move(iterator from, iterator to) {
        if (m_cur_pos) {
            auto p = begin() + m_cur_pos.value();
            if (p > from && p < to)
                m_cur_pos = m_cur_pos.value() + 1;
            else if (p == from)
                m_cur_pos = (usize)std::distance(begin(), to);
        }
        return std::rotate(from, from + 1, to);
    }
    void clear() {
        m_cur_pos = std::nullopt;
        m_list.clear();
    }

    void erase(const QString& s) {
        auto it = pos_it(s);
        if (it != end()) erase(it);
    }
    const auto& at(usize p) const { return m_list.at(p); }

    void set_cur(const QString& s) {
        const auto& self = *this;
        auto        it   = pos_it(s);
        if (it != end()) m_cur_pos = (usize)std::distance(self.begin(), it);
    }

    void try_set_cur_frist() {
        if (! m_list.empty()) m_cur_pos = 0;
    }

    bool next(bool loop) {
        if (m_cur_pos) {
            auto p    = m_cur_pos.value();
            auto s    = m_list.size();
            auto n    = p + 1;
            m_cur_pos = n >= s ? (loop ? n % s : p) : n;
            return m_cur_pos.value() != p;
        }
        return false;
    }

    bool prev(bool loop) {
        if (m_cur_pos) {
            auto p    = m_cur_pos.value();
            auto s    = m_list.size();
            m_cur_pos = p == 0 ? (loop ? s - 1 : 0) : p - 1;
            return m_cur_pos.value() != p;
        }
        return false;
    }

    void sync_cur(const PlayList& o) {
        if (auto cur = o.cur())
            set_cur(cur.value());
        else
            m_cur_pos = std::nullopt;
    }

    void shuffle(iterator from, iterator end);
    auto random_insert_after(iterator it, const QString& s);

private:
    std::vector<QString> m_list;
    std::optional<usize> m_cur_pos;
};

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

    struct CurGuard {
        CurGuard(Playlist& p): p(p) {
            auto& list = p.m_list;
            cur        = list.cur();
            cur_pos    = list.cur_pos();
        }
        ~CurGuard() {
            auto& list = p.m_list;
            if (cur_pos != list.cur_pos())
                emit p.curIndexChanged();
            else if (cur != list.cur())
                p.check_cur();
        }

        Playlist&              p;
        std::optional<QString> cur;
        std::optional<usize>   cur_pos;
    };

    Playlist(QObject* parent = nullptr);
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
    void curIndexChanged();
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
    void check_cur();
    void RefreshCanMove();

private:
    detail::PlayList& oper_list();
    detail::PlayList& sync_list();

    model::Song                    m_cur;
    std::map<QString, model::Song> m_songs;
    detail::PlayList               m_list;
    detail::PlayList               m_shuffle_list;
    LoopMode                       m_loop_mode;
    bool                           m_can_next;
    bool                           m_can_prev;
};
} // namespace qcm
