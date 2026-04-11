module;
#include "Qcm/model/play_queue.moc.h"
#include "Qcm/model/app_info.moc.h"
#include "Qcm/model/id_queue.moc.h"
#include "Qcm/model/router_msg.moc.h"
#include "Qcm/model/empty_model.moc.h"
#include "Qcm/model/store_item.moc.h"
#include "Qcm/model/list_models.moc.h"
#include "Qcm/model/page_model.moc.h"
#include "Qcm/model/table_proxy.moc.h"
module qcm;
import :model;

namespace qcm::model
{
AppInfo::AppInfo() {
    this->name    = APP_NAME;
    this->id      = APP_ID;
    this->author  = APP_AUTHOR;
    this->summary = APP_SUMMARY;
    this->version = APP_VERSION;
}
AppInfo::~AppInfo() {}

} // namespace qcm::model
namespace qcm::model
{

SongStoreItem::SongStoreItem(QObject* parent): base_type(AppStore::instance()->songs, parent) {}
AlbumStoreItem::AlbumStoreItem(QObject* parent): base_type(AppStore::instance()->albums, parent) {}
ArtistStoreItem::ArtistStoreItem(QObject* parent)
    : base_type(AppStore::instance()->artists, parent) {}
MixStoreItem::MixStoreItem(QObject* parent): base_type(AppStore::instance()->mixes, parent) {}

} // namespace qcm::model


namespace qcm::model
{
AlbumListModel::AlbumListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent, kstore::QMetaRoleNames::WithMethod),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }) {}
QQmlPropertyMap* AlbumListModel::extra(i32 idx) const {
    if (auto extend = AppStore::instance()->albums.query_extend(this->key_at(idx)); extend) {
        return extend->extra.get();
    }
    return nullptr;
}
SongListModel::SongListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent, kstore::QMetaRoleNames::WithMethod),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }) {}
QQmlPropertyMap* SongListModel::extra(i32 idx) const {
    if (auto extend = AppStore::instance()->songs.query_extend(this->key_at(idx)); extend) {
        return extend->extra.get();
    }
    return nullptr;
}

AlbumSongListModel::AlbumSongListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent, kstore::QMetaRoleNames::WithMethod),
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
    : kstore::QGadgetListModel(this, parent, kstore::QMetaRoleNames::WithMethod),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }) {}

MixListModel::MixListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent, kstore::QMetaRoleNames::WithMethod),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }) {}

RadioQueueListModel::RadioQueueListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent, kstore::QMetaRoleNames::WithMethod) {}

MixSongListModel::MixSongListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent, kstore::QMetaRoleNames::WithMethod),
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

namespace qcm
{

void PageModel::init_main_pages(PageModel* self) {
    std::array arr { Page { .name   = "home",
                            .icon   = "home",
                            .source = "qrc:/Qcm/App/qml/page/HomePage.qml",
                            .cache  = true },
                     Page { .name   = "library",
                            .icon   = "library_music",
                            .source = "qrc:/Qcm/App/qml/page/LibraryPage.qml",
                            .cache  = true },
                     Page { .name   = "search",
                            .icon   = "search",
                            .source = "qrc:/Qcm/App/qml/page/SearchPage.qml" } };
    self->insert(0, arr);
#ifdef QCM_DEBUG_BUILD
    self->insert(self->rowCount(),
                 Page { .name   = "test",
                        .icon   = "bug_report",
                        .source = "qrc:/Qcm/Material/Example/Example.qml" });
#endif
}
PageModel::PageModel(QObject* parent): kstore::QGadgetListModel(this, parent) {}
PageModel::~PageModel() {}
} // namespace qcm


#include "Qcm/model/app_info.moc.cpp"
#include "Qcm/model/id_queue.moc.cpp"
#include "Qcm/model/router_msg.moc.cpp"
#include "Qcm/model/empty_model.moc.cpp"
#include "Qcm/model/store_item.moc.cpp"
#include "Qcm/model/list_models.moc.cpp"
#include "Qcm/model/page_model.moc.cpp"
#include "Qcm/model/table_proxy.moc.cpp"
#include "Qcm/model/play_queue.moc.cpp"