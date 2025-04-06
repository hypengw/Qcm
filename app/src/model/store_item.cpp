#include "Qcm/model/store_item.hpp"

#include "Qcm/store.hpp"

namespace qcm::model
{

SongStoreItem::SongStoreItem(QObject* parent): base_type(AppStore::instance()->songs, parent) {}
AlbumStoreItem::AlbumStoreItem(QObject* parent): base_type(AppStore::instance()->albums, parent) {}
ArtistStoreItem::ArtistStoreItem(QObject* parent)
    : base_type(AppStore::instance()->artists, parent) {}
MixStoreItem::MixStoreItem(QObject* parent): base_type(AppStore::instance()->mixes, parent) {}

} // namespace qcm::model

#include <Qcm/model/moc_store_item.cpp>