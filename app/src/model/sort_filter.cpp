#include "Qcm/model/sort_filter.hpp"
#include "Qcm/backend_msg.hpp"
#include <array>
#include "core/log.h"

namespace qcm
{

SortTypeModel::SortTypeModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent), m_current_idx(0), m_asc(true) {}

auto SortTypeModel::currentIndex() const -> qint32 { return m_current_idx; }
auto SortTypeModel::currentType() const -> qint32 {
    if (m_current_idx < 0 || m_current_idx > rowCount()) {
        return 0;
    }
    return at(m_current_idx).type;
}
void SortTypeModel::setCurrentIndex(qint32 v) {
    if (m_current_idx != v) {
        m_current_idx = v;
        currentIndexChanged();
        currentTypeChanged();
    }
}
void SortTypeModel::setCurrentType(qint32 t) {
    for (usize i = 0; i < this->size(); i++) {
        auto& el = at(i);
        if (el.type == t) {
            setCurrentIndex(i);
            break;
        }
    }
}

auto SortTypeModel::asc() const -> bool { return m_asc; }
void SortTypeModel::setAsc(bool v) {
    if (m_asc != v) {
        m_asc = v;
        ascChanged();
    }
}

FilterTypeModel::FilterTypeModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent), m_current_idx(0) {}

auto FilterTypeModel::currentIndex() const -> qint32 { return m_current_idx; }
auto FilterTypeModel::currentType() const -> qint32 {
    if (m_current_idx < 0 || m_current_idx > rowCount()) {
        return 0;
    }
    return at(m_current_idx).type;
}
void FilterTypeModel::setCurrentIndex(qint32 v) {
    if (m_current_idx != v) {
        m_current_idx = v;
        currentIndexChanged();
        currentTypeChanged();
    }
}
void FilterTypeModel::setCurrentType(qint32 t) {
    for (usize i = 0; i < this->size(); i++) {
        auto& el = at(i);
        if (el.type == t) {
            setCurrentIndex(i);
            break;
        }
    }
}

AlbumSortTypeModel::AlbumSortTypeModel(QObject* parent): SortTypeModel(parent) {
    using E = msg::model::AlbumSortGadget::AlbumSort;
    this->insert(
        0,
        std::array {
            SortTypeItem { .type = (qint32)E::ALBUM_SORT_TITLE, .name = "title" },
            SortTypeItem { .type = (qint32)E::ALBUM_SORT_SORT_TITLE, .name = "sort title" },
            SortTypeItem { .type = (qint32)E::ALBUM_SORT_YEAR, .name = "year" },
            SortTypeItem { .type = (qint32)E::ALBUM_SORT_TRACK_COUNT, .name = "track count" },
            SortTypeItem { .type = (qint32)E::ALBUM_SORT_DISC_COUNT, .name = "disc count" },
            SortTypeItem { .type = (qint32)E::ALBUM_SORT_PUBLISH_TIME, .name = "publish time" },
            SortTypeItem { .type = (qint32)E::ALBUM_SORT_ADDED_TIME, .name = "added time" },
        });
}
ArtistSortTypeModel::ArtistSortTypeModel(QObject* parent): SortTypeModel(parent) {
    using E = msg::model::ArtistSortGadget::ArtistSort;
    this->insert(
        0,
        std::array {
            SortTypeItem { .type = (qint32)E::ARTIST_SORT_NAME, .name = "name" },
            SortTypeItem { .type = (qint32)E::ARTIST_SORT_SORT_NAME, .name = "sort name" },
            SortTypeItem { .type = (qint32)E::ARTIST_SORT_ALBUM_COUNT, .name = "album count" },
            SortTypeItem { .type = (qint32)E::ARTIST_SORT_MUSIC_COUNT, .name = "music count" },
        });
}

SongSortTypeModel::SongSortTypeModel(QObject* parent): SortTypeModel(parent) {
    using E = msg::model::SongSortGadget::SongSort;
    this->insert(
        0,
        std::array {
            SortTypeItem { .type = (qint32)E::SONG_SORT_TRACK_NUMBER, .name = "track number" },
            SortTypeItem { .type = (qint32)E::SONG_SORT_TITLE, .name = "title" },
            SortTypeItem { .type = (qint32)E::SONG_SORT_SORT_TITLE, .name = "sort title" },
            SortTypeItem { .type = (qint32)E::SONG_SORT_PUBLISH_TIME, .name = "publish time" },
            SortTypeItem { .type = (qint32)E::SONG_SORT_DURATION, .name = "duration" },
            SortTypeItem { .type = (qint32)E::SONG_SORT_POPULARITY, .name = "popularity" },
        });
}

SongSortFilterModel::SongSortFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent), m_sort_type(0), m_asc(true), m_disc_role(-1) {
    connect(
        this, &SongSortFilterModel::sourceModelChanged, this, &SongSortFilterModel::freshSortType);
    connect(this, &SongSortFilterModel::sortTypeChanged, this, &SongSortFilterModel::freshSortType);
    connect(this, &SongSortFilterModel::sortRoleChanged, this, &SongSortFilterModel::freshSort);
    connect(this, &SongSortFilterModel::ascChanged, this, &SongSortFilterModel::freshSort);
}

void SongSortFilterModel::freshSort() { sort(0, m_asc ? Qt::AscendingOrder : Qt::DescendingOrder); }
void SongSortFilterModel::freshSortType() {
    using E = msg::model::SongSortGadget::SongSort;
    if (auto s = sourceModel()) {
        if (s->columnCount() <= 1) {
            QHash<QByteArray, qint32> name_maps;
            for (const auto& el : s->roleNames().asKeyValueRange()) {
                name_maps.insert_or_assign(el.second, el.first);
            }
            std::string_view name;
            switch ((E)m_sort_type) {
            case E::SONG_SORT_TITLE: name = "name"; break;
            case E::SONG_SORT_SORT_TITLE: name = "sortName"; break;
            case E::SONG_SORT_PUBLISH_TIME: name = "publishTime"; break;
            case E::SONG_SORT_DURATION: name = "duration"; break;
            case E::SONG_SORT_POPULARITY: name = "popularity"; break;
            case E::SONG_SORT_TRACK_NUMBER:
            default: name = "trackNumber"; break;
            }

            if (auto it = name_maps.find("discNumber"); it != name_maps.end()) {
                m_disc_role = *it;
            }

            if (auto it = name_maps.find(name.data()); it != name_maps.end()) {
                setSortRole(*it);
            } else {
                log::error("{}", name.data());
            }
        }
    }
}

bool SongSortFilterModel::lessThan(const QModelIndex& source_left,
                                   const QModelIndex& source_right) const {
    using E = msg::model::SongSortGadget::SongSort;

    auto disc_left  = 0;
    auto disc_right = 0;
    if (m_disc_role >= 0) {
        disc_left  = source_left.data(m_disc_role).toInt();
        disc_right = source_right.data(m_disc_role).toInt();
    }

    if (disc_left == disc_right) {
        if (m_sort_type == (qint32)E::SONG_SORT_TRACK_NUMBER) {
            return source_left.data(sortRole()).toInt() < source_right.data(sortRole()).toInt();
        }
    } else {
        return disc_left < disc_right;
    }
    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

auto SongSortFilterModel::asc() const -> bool { return m_asc; }
void SongSortFilterModel::setAsc(bool v) {
    if (m_asc != v) {
        m_asc = v;
        ascChanged();
    }
}

auto SongSortFilterModel::sortType() const -> qint32 { return m_sort_type; }
void SongSortFilterModel::setSortType(qint32 v) {
    if (m_sort_type != v) {
        m_sort_type = v;
        sortTypeChanged();
    }
}

AlbumFilterTypeModel::AlbumFilterTypeModel(QObject* parent): FilterTypeModel(parent) {
    using E = msg::filter::FilterTypeGadget::FilterType;
    this->insert(
        0,
        std::array {
            FilterTypeItem { .type = (qint32)E::FILTER_TYPE_TITLE, .name = "title" },
            FilterTypeItem { .type = (qint32)E::FILTER_TYPE_TRACK_COUNT, .name = "track count" },
            FilterTypeItem { .type = (qint32)E::FILTER_TYPE_YEAR, .name = "year" },
            FilterTypeItem { .type = (qint32)E::FILTER_TYPE_DURATION, .name = "duration" },
            FilterTypeItem { .type = (qint32)E::FILTER_TYPE_TYPE, .name = "type" },
            FilterTypeItem { .type = (qint32)E::FILTER_TYPE_DISC_COUNT, .name = "disc count" },
            FilterTypeItem { .type = (qint32)E::FILTER_TYPE_ARTIST_NAME, .name = "artist name" },
        });
}

ArtistFilterTypeModel::ArtistFilterTypeModel(QObject* parent): FilterTypeModel(parent) {
    using E = msg::filter::FilterTypeGadget::FilterType;
    this->insert(
        0,
        std::array {
            FilterTypeItem { .type = (qint32)E::FILTER_TYPE_NAME, .name = "name" },
            FilterTypeItem { .type = (qint32)E::FILTER_TYPE_YEAR, .name = "year" },
            FilterTypeItem { .type = (qint32)E::FILTER_TYPE_ALBUM_TITLE, .name = "album title" },
        });
}

} // namespace qcm

#include "Qcm/model/moc_sort_filter.cpp"