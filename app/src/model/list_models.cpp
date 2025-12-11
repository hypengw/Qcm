#include "Qcm/model/list_models.hpp"
#include "Qcm/model/store_item.hpp"
#include "Qcm/store.hpp"

#include "Qcm/util/mem.hpp"

namespace qcm::model
{
AlbumListModel::AlbumListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }) {}
QQmlPropertyMap* AlbumListModel::extra(i32 idx) const {
    if (auto extend = AppStore::instance()->albums.query_extend(this->key_at(idx)); extend) {
        return extend->extra.get();
    }
    return nullptr;
}
SongListModel::SongListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }) {}
QQmlPropertyMap* SongListModel::extra(i32 idx) const {
    if (auto extend = AppStore::instance()->songs.query_extend(this->key_at(idx)); extend) {
        return extend->extra.get();
    }
    return nullptr;
}

AlbumSongListModel::AlbumSongListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }),
      m_disc_count(1) {
    connect(&m_item, &model::AlbumStoreItem::itemChanged, this, &AlbumSongListModel::albumChanged);
}
AlbumSongListModel::~AlbumSongListModel() {}
auto AlbumSongListModel::album() const -> album_type { return m_item.item(); }
auto AlbumSongListModel::discCount() const -> qint32 { return m_disc_count; }
void AlbumSongListModel::setAlbum(const album_type& album) { m_item.setItem(album); }
void AlbumSongListModel::setDiscCount(qint32 c) {
    if (c != m_disc_count) {
        m_disc_count = c;
        discCountChanged();
    }
}
auto AlbumSongListModel::extra() const -> QQmlPropertyMap* { return m_item.extra(); }

ArtistListModel::ArtistListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }) {}

MixListModel::MixListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }) {}

MixSongListModel::MixSongListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }),
      m_disc_count(1) {
    connect(&m_item, &model::MixStoreItem::itemChanged, this, &MixSongListModel::mixChanged);
}
MixSongListModel::~MixSongListModel() {}
auto MixSongListModel::mix() const -> mix_type { return m_item.item(); }
auto MixSongListModel::discCount() const -> qint32 { return m_disc_count; }
void MixSongListModel::setMix(const mix_type& mix) { m_item.setItem(mix); }
void MixSongListModel::setDiscCount(qint32 c) {
    if (c != m_disc_count) {
        m_disc_count = c;
        discCountChanged();
    }
}
auto MixSongListModel::extra() const -> QQmlPropertyMap* { return m_item.extra(); }

} // namespace qcm::model

#include <Qcm/model/moc_list_models.cpp>