#include "Qcm/model/list_models.hpp"
#include "Qcm/model/store_item.hpp"
#include "Qcm/store.hpp"

namespace qcm::model
{
AlbumListModel::AlbumListModel(QObject* parent): base_type(parent) {}
QQmlPropertyMap* AlbumListModel::extra(i32 idx) const {
    if (auto extend = AppStore::instance()->albums.query_extend(this->key_at(idx)); extend) {
        return extend->extra.get();
    }
    return nullptr;
}

AlbumSongListModel::AlbumSongListModel(QObject* parent): base_type(parent) {
    connect(&m_item, &model::AlbumStoreItem::itemChanged, this, &AlbumSongListModel::albumChanged);
}
AlbumSongListModel::~AlbumSongListModel() {}
auto AlbumSongListModel::album() const -> album_type { return m_item.item(); }
void AlbumSongListModel::setAlbum(const album_type& album) { m_item.setItem(album); }
auto AlbumSongListModel::extra() const -> QQmlPropertyMap* { return m_item.extra(); }

ArtistListModel::ArtistListModel(QObject* parent): base_type(parent) {}

MixListModel::MixListModel(QObject* parent): base_type(parent) {}
} // namespace qcm::model

#include <Qcm/model/moc_list_models.cpp>