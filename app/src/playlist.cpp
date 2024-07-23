#include "Qcm/playlist.h"

#include <ranges>

#include "core/random.h"
#include "core/log.h"

using namespace qcm;

namespace
{

inline QString get_song_id_str(const model::ItemId& id) { 
    auto url = id.toUrl();
    auto out = url.toString(); 
    _assert_(!out.isEmpty());
    return out;
}
inline QString get_song_id_str(const model::Song& s) { return get_song_id_str(s.id); }

} // namespace

namespace qcm::detail
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
    auto pos_it(const QString& s) {
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
        auto* p    = &s;
        auto* pend = p + 1;
        return m_list.insert(it, p, pend);
    }
    template<typename Tin>
    auto insert(const_iterator it, Tin&& beg, Tin&& end) {
        const auto& self = *this;
        auto        d    = (usize)std::distance(self.begin(), it);

        auto old_size = size();
        auto out      = m_list.insert(it, beg, end);
        auto in_size  = size() - old_size;

        if (m_cur_pos) {
            auto p = m_cur_pos.value();
            if (d <= p) m_cur_pos = p + in_size;
        }
        return out;
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
        auto it = pos_it(s);
        if (it != end()) m_cur_pos = (usize)std::distance(begin(), it);
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

    template<typename Tin>
    auto random_insert_after(iterator it, Tin&& beg, Tin&& end) {
        auto& self = *this;
        auto  d    = (usize)std::distance(self.begin(), it);
        auto  out  = insert(it, beg, end);
        shuffle(this->begin() + d, this->end());
        return out;
    }

private:
    std::vector<QString> m_list;
    std::optional<usize> m_cur_pos;
};

} // namespace qcm::detail

struct qcm::Playlist::CurGuard {
    CurGuard(Playlist& p, bool refresh = false): p(p), refresh(refresh) {
        auto& list = p.m_list;
        cur        = list->cur();
        cur_pos    = list->cur_pos();
    }
    ~CurGuard() {
        auto& list = p.m_list;
        if (cur_pos != list->cur_pos())
            emit p.curIndexChanged(refresh);
        else if (cur != list->cur())
            p.check_cur(refresh);
    }

    Playlist&              p;
    bool                   refresh;
    std::optional<QString> cur;
    std::optional<usize>   cur_pos;
};

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

Playlist::Playlist(QObject* parent)
    : QAbstractListModel(parent),
      m_list(make_up<detail::PlayList>()),
      m_shuffle_list(make_up<detail::PlayList>()),
      m_loop_mode(LoopMode::NoneLoop),
      m_can_next(false),
      m_can_prev(false) {
    connect(this, &Playlist::curIndexChanged, this, &Playlist::check_cur, Qt::DirectConnection);
    connect(this, &Playlist::loopModeChanged, this, &Playlist::RefreshCanMove);
    connect(this, &Playlist::curIndexChanged, this, &Playlist::RefreshCanMove);
    connect(this, &QAbstractItemModel::rowsInserted, this, &Playlist::RefreshCanMove);
}

Playlist::~Playlist() {}

template<typename T>
    requires std::ranges::sized_range<T> &&
             std::convertible_to<std::ranges::range_value_t<T>, model::Song>
usize Playlist::insert(int index, const T& range) {
    auto filter = [this](const model::Song& s) -> bool {
        return ! m_songs.contains(s.id);
    };
    std::vector<QString> ids;

    for (auto& s : range | std::views::filter(filter)) {
        m_songs.insert({ s.id, s });
        ids.emplace_back(get_song_id_str(s));
    }
    auto size = ids.size();
    if (size == 0) return size;

    beginInsertRows({}, index, index + size - 1);
    {
        auto cur = m_list->cur();
        m_list->insert(m_list->begin() + index, std::begin(ids), std::end(ids));
        auto it = cur ? m_shuffle_list->pos_it(cur.value()) + 1 : m_shuffle_list->begin() + index;
        _assert_rel_(it <= m_shuffle_list->end());
        if (std::size(ids) > 1)
            m_shuffle_list->random_insert_after(it, std::begin(ids), std::end(ids));
        else
            m_shuffle_list->insert(it, std::begin(ids), std::end(ids));
    }
    endInsertRows();

    {
        auto& list = oper_list();
        if (! list.cur_ok()) {
            list.try_set_cur_frist();
            sync_list().sync_cur(list);
        }
    }

    return size;
}

int Playlist::rowCount(const QModelIndex&) const { return m_list->size(); }

QVariant Playlist::data(const QModelIndex& index, int role) const {
    auto row = index.row();
    if (row >= rowCount()) return {};
    switch (role) {
    case SongRole:
        return QVariant::fromValue(m_songs.at(model::ItemId(QUrl(m_list->at(row)))));
        break;
    }
    return {};
};

QHash<int, QByteArray> Playlist::roleNames() const { return { { SongRole, "song" } }; }

std::optional<QString> Playlist::cur_id() const { return oper_list().cur(); }

const model::Song& Playlist::cur() const { return m_cur; }
qint32             Playlist::curIndex() const {
    auto cur = m_list->cur_pos();
    return cur ? (int)cur.value() : -1;
}

Playlist::LoopMode Playlist::loopMode() const { return m_loop_mode; }
void               Playlist::setLoopMode(LoopMode v) {
    auto old = m_loop_mode;
    if (std::exchange(m_loop_mode, v) != v) {
        if (v == ShuffleLoop) {
            *m_shuffle_list = *m_list;
            m_shuffle_list->shuffle(m_shuffle_list->begin(), m_shuffle_list->end());
            m_shuffle_list->sync_cur(*m_list);
        }
        if (old == ShuffleLoop) {
            m_list->sync_cur(*m_shuffle_list);
        }
        emit loopModeChanged();
    }
}

bool Playlist::canNext() const { return m_can_next; }
bool Playlist::canPrev() const { return m_can_prev; }
void Playlist::setCanNext(bool v) {
    if (std::exchange(m_can_next, v) != v) {
        emit canMoveChanged();
    }
}
void Playlist::setCanPrev(bool v) {
    if (std::exchange(m_can_prev, v) != v) {
        emit canMoveChanged();
    }
}

void Playlist::switchList(const std::vector<model::Song>& songs) {
    auto old_id = m_cur.id;
    clear();

    insert(0, songs);
    emit curIndexChanged(true);
}

void Playlist::switchTo(const model::Song& song) {
    auto song_id = get_song_id_str(song);
    if (! m_songs.contains(song.id)) {
        appendNext(song);
    }
    m_list->set_cur(song_id);
    m_shuffle_list->set_cur(song_id);

    emit curIndexChanged(true);
}

void Playlist::appendNext(const model::Song& song) {
    remove(song.id);

    auto pos = std::distance(m_list->begin(), m_list->cur_it());
    if (pos != rowCount()) pos++;
    insert(pos, std::array { song });
}

void Playlist::append(const model::Song& song) { insert(rowCount(), std::array { song }); }

void Playlist::remove(model::ItemId id) {
    auto song_id = get_song_id_str(id);
    if (! m_songs.contains(id)) return;
    CurGuard gd { *this };

    auto pos = m_list->pos(song_id).value();
    beginRemoveRows({}, pos, pos);

    m_list->erase(m_list->begin() + pos);
    m_shuffle_list->erase(song_id);
    m_songs.erase(id);

    endRemoveRows();
}

void Playlist::appendList(const std::vector<model::Song>& songs) {
    insert(rowCount(), songs);
    emit curIndexChanged(false);
}

void Playlist::clear() {
    CurGuard gd { *this };
    beginResetModel();
    m_list->clear();
    m_shuffle_list->clear();
    m_songs.clear();
    endResetModel();
}

void Playlist::next() {
    CurGuard gd { *this, true };

    auto& list = oper_list();

    switch (m_loop_mode) {
    case NoneLoop: list.next(false); break;
    case ShuffleLoop:
    case ListLoop:
        list.next(true);
        if (rowCount() == 1) {
            emit curChanged(true);
        }
        break;
    case SingleLoop: emit curChanged(true); break;
    }
    sync_list().sync_cur(list);
}
void Playlist::prev() {
    CurGuard gd { *this, true };

    auto& list = oper_list();

    switch (m_loop_mode) {
    case NoneLoop: list.prev(false); break;
    case ShuffleLoop:
    case ListLoop:
        list.prev(true);
        if (rowCount() == 1) {
            emit curChanged(true);
        }
        break;
    case SingleLoop: emit curChanged(true); break;
    }
    sync_list().sync_cur(list);
}

void Playlist::check_cur(bool refresh) {
    auto old_id = std::make_optional(get_song_id_str(m_cur));
    if (auto cur = cur_id(); cur != old_id) {
        if (cur)
            m_cur = m_songs.at(model::ItemId(cur.value()));
        else
            m_cur = {};

        emit curChanged(refresh);
    }
}
void Playlist::RefreshCanMove() {
    auto& list = oper_list();
    if (list.size() == 0) {
        setCanNext(false);
        setCanPrev(false);
    } else if (m_loop_mode == LoopMode::NoneLoop) {
        auto it = list.cur_it();
        setCanNext(it + 1 != list.end());
        setCanPrev(it != list.begin());
    } else {
        setCanNext(true);
        setCanPrev(true);
    }
}

detail::PlayList& Playlist::oper_list() {
    return m_loop_mode == ShuffleLoop ? *m_shuffle_list : *m_list;
}
detail::PlayList& Playlist::sync_list() {
    return m_loop_mode != ShuffleLoop ? *m_shuffle_list : *m_list;
}
const detail::PlayList& Playlist::oper_list() const {
    return m_loop_mode == ShuffleLoop ? *m_shuffle_list : *m_list;
}
const detail::PlayList& Playlist::sync_list() const {
    return m_loop_mode != ShuffleLoop ? *m_shuffle_list : *m_list;
}
