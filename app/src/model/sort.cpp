#include "Qcm/model/sort.hpp"
#include "Qcm/backend_msg.hpp"
#include <array>

namespace qcm
{

SortTypeModel::SortTypeModel(QObject* parent): Base(parent), m_current_idx(0), m_asc(true) {}

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
auto SortTypeModel::asc() const -> bool { return m_asc; }
void SortTypeModel::setAsc(bool v) {
    if (m_asc != v) {
        m_asc = v;
        ascChanged();
    }
}

AlbumSortTypeModel::AlbumSortTypeModel(QObject* parent): SortTypeModel(parent) {
    using E = msg::model::AlbumSortGadget::AlbumSort;
    this->insert(0,
                 std::array {
                     SortTypeItem { .type = (qint32)E::TITLE, .name = "title" },
                     SortTypeItem { .type = (qint32)E::SORT_TITLE, .name = "sort title" },
                     SortTypeItem { .type = (qint32)E::YEAR, .name = "year" },
                     SortTypeItem { .type = (qint32)E::TRACK_COUNT, .name = "track count" },
                     SortTypeItem { .type = (qint32)E::PUBLISH_TIME, .name = "publish time" },
                 });
}
ArtistSortTypeModel::ArtistSortTypeModel(QObject* parent): SortTypeModel(parent) {
    using E = msg::model::ArtistSortGadget::ArtistSort;
    this->insert(0,
                 std::array {
                     SortTypeItem { .type = (qint32)E::NAME, .name = "name" },
                     SortTypeItem { .type = (qint32)E::SORT_NAME, .name = "sort name" },
                     SortTypeItem { .type = (qint32)E::ALBUM_COUNT, .name = "album count" },
                     SortTypeItem { .type = (qint32)E::MUSIC_COUNT, .name = "music count" },
                 });
}

} // namespace qcm

#include "Qcm/model/moc_sort.cpp"