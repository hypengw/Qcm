#include "Qcm/playlist.h"

#include "core/random.h"

using namespace qcm;

namespace
{

const QString& sid(const model::Song& s) { return s.id.id; }

} // namespace

void detail::PlayList::shuffle(iterator from, iterator end) {
    auto cur_ = cur();
    Random::shuffle(from, end);

    if (cur_) set_cur(cur_.value());
}

auto detail::PlayList::random_insert_after(iterator it, const QString& s) {
    auto end_      = end();
    auto random_it = Random::get(it == end_ ? end_ : it + 1, end_);
    return insert(random_it, s);
}

Playlist::Playlist(QObject* parent): QAbstractListModel(parent), m_loop_mode(LoopMode::NoneLoop) {
    connect(this, &Playlist::curIndexChanged, this, &Playlist::check_cur, Qt::DirectConnection);
}

int Playlist::rowCount(const QModelIndex&) const { return m_list.size(); }

QVariant Playlist::data(const QModelIndex& index, int role) const {
    auto row = index.row();
    if (row >= rowCount()) return {};
    switch (role) {
    case SongRole: return QVariant::fromValue(m_songs.at(m_list.at(row))); break;
    }
    return {};
};

QHash<int, QByteArray> Playlist::roleNames() const { return { { SongRole, "song" } }; }

std::optional<QString> Playlist::cur_id() const {
    if (loopMode() == ShuffleLoop)
        return m_shuffle_list.cur();
    else
        return m_list.cur();
}

const model::Song& Playlist::cur() const { return m_cur; }
qint32             Playlist::curIndex() const {
    auto cur = m_list.cur_pos();
    return cur ? (int)cur.value() : -1;
}

Playlist::LoopMode Playlist::loopMode() const { return m_loop_mode; }
void               Playlist::setLoopMode(LoopMode v) {
    auto old = m_loop_mode;
    if (std::exchange(m_loop_mode, v) != v) {
        if (v == ShuffleLoop) {
            m_shuffle_list = m_list;
            m_shuffle_list.shuffle(m_shuffle_list.begin(), m_shuffle_list.end());
            m_shuffle_list.sync_cur(m_list);
        }
        if (old == ShuffleLoop) {
            m_list.sync_cur(m_shuffle_list);
        }
        emit loopModeChanged();
    }
}

void Playlist::switchList(const std::vector<model::Song>& songs) {
    auto old_id = m_cur.id;
    clear();

    appendList(songs);
    m_shuffle_list.shuffle(m_shuffle_list.begin(), m_shuffle_list.end());

    {
        auto& list = oper_list();
        list.try_set_cur_frist();
        sync_list().sync_cur(list);
    }

    emit curIndexChanged();
    if (m_cur.id == old_id) emit curChanged(true);
}

void Playlist::switchTo(const model::Song& song) {
    append(song);
    m_list.set_cur(sid(song));
    m_shuffle_list.set_cur(sid(song));

    auto old_id = m_cur.id;
    emit curIndexChanged();
    if (m_cur.id == old_id) emit curChanged(true);
}

void Playlist::appendNext(const model::Song& song) {
    remove(sid(song));

    auto pos = std::distance(m_list.begin(), m_list.cur_it());
    if (pos != rowCount()) pos++;
    beginInsertRows({}, pos, pos);
    m_list.insert(m_list.begin() + pos, sid(song));
    {
        auto cur_ = m_shuffle_list.cur_it();
        m_shuffle_list.insert(cur_ == m_shuffle_list.end() ? cur_ : cur_ + 1, sid(song));
    }
    m_songs.insert({ sid(song), song });

    endInsertRows();
}

void Playlist::append(const model::Song& song) {
    if (m_songs.contains(sid(song))) return;

    beginInsertRows({}, rowCount(), rowCount());

    m_list.insert(m_list.end(), sid(song));
    m_shuffle_list.random_insert_after(m_shuffle_list.cur_it(), sid(song));
    m_songs.insert({ sid(song), song });

    endInsertRows();
}

void Playlist::remove(model::SongId id) {
    if (! m_songs.contains(id.id)) return;
    CurGuard gd { *this };

    auto pos = m_list.pos(id.id).value();
    beginRemoveRows({}, pos, pos);

    m_list.erase(m_list.begin() + pos);
    m_shuffle_list.erase(id.id);
    m_songs.erase(id.id);

    endRemoveRows();
}

void Playlist::appendList(const std::vector<model::Song>& songs) {
    for (const auto& s : songs) {
        append(s);
    }
}

void Playlist::clear() {
    CurGuard gd { *this };
    beginResetModel();
    m_list.clear();
    m_songs.clear();
    m_shuffle_list.clear();
    endResetModel();
}

void Playlist::next() {
    CurGuard gd { *this };

    auto& list = oper_list();

    switch (m_loop_mode) {
    case NoneLoop: list.next(false); break;
    case ShuffleLoop:
    case ListLoop:
        list.next(true);
        if (rowCount() == 1) emit curChanged(true);
        break;
    case SingleLoop: emit curChanged(true); break;
    }
    sync_list().sync_cur(list);
}
void Playlist::prev() {
    CurGuard gd { *this };

    auto& list = oper_list();

    switch (m_loop_mode) {
    case NoneLoop: list.prev(false); break;
    case ShuffleLoop:
    case ListLoop:
        list.prev(true);
        if (rowCount() == 1) emit curChanged(true);
        break;
    case SingleLoop: emit curChanged(true); break;
    }
    sync_list().sync_cur(list);
}

void Playlist::check_cur() {
    auto old_id = std::make_optional(m_cur.id);
    if (auto cur = cur_id(); cur != old_id) {
        if (cur)
            m_cur = m_songs.at(cur.value());
        else
            m_cur = {};

        emit curChanged();
    }
}
detail::PlayList& Playlist::oper_list() {
    return m_loop_mode == ShuffleLoop ? m_shuffle_list : m_list;
}
detail::PlayList& Playlist::sync_list() {
    return m_loop_mode != ShuffleLoop ? m_shuffle_list : m_list;
}
