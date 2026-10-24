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
AlbumListModel::AlbumListModel(QObject* parent)
    : EntityList<model::Album>(parent,enums::ItemType::ItemAlbum,AppStore::instance()->albums) {}

SongListModel::SongListModel(QObject* parent)
    : EntityList<model::Song>(parent,enums::ItemType::ItemSong,AppStore::instance()->songs) {}


AlbumSongListModel::AlbumSongListModel(QObject* parent)
    : EntityList<model::Song>(parent,enums::ItemType::ItemSong,AppStore::instance()->songs),
      m_disc_count(1) {
    connect(&m_item, &EntityView::changed, this, &AlbumSongListModel::albumChanged);
}
AlbumSongListModel::~AlbumSongListModel() {}
auto AlbumSongListModel::album() const -> album_type { return m_item.item(); }
auto AlbumSongListModel::discCount() const -> qint32 { return m_disc_count; }
void AlbumSongListModel::setAlbumId(i64 id) { m_item.setItemId(id); }
void AlbumSongListModel::setDiscCount(qint32 c) {
    if (c != m_disc_count) {
        m_disc_count = c;
        discCountChanged();
    }
}

ArtistListModel::ArtistListModel(QObject* parent)
    : EntityList<model::Artist>(parent,enums::ItemType::ItemAlbumArtist,AppStore::instance()->artists) {}

MixListModel::MixListModel(QObject* parent)
    : EntityList<model::Mix>(parent,enums::ItemType::ItemMix,AppStore::instance()->mixes) {}

ForYouListModel::ForYouListModel(QObject* parent): kstore::QGadgetListModel(this, parent) {}
auto ForYouListModel::data(const QModelIndex& index, int role) const -> QVariant {
    if (index.row() < 0 || index.row() >= rowCount()) return {};
    const auto& card = at(index.row());
    switch (role) {
    case Qt::UserRole + 1024: return QVariant::fromValue(card);
    case Qt::UserRole + 1025: return QVariant::fromValue(item_id(card));
    case Qt::UserRole + 1026: return QVariant::fromValue(cover_id(card));
    case Qt::UserRole + 1027:
        if (card.hasSubtitle()) return card.subtitle();
        if (card.hasMix()) return tr("%1 songs").arg(static_cast<int>(card.mix().trackCount()));
        if (card.hasAlbum()) return tr("%1 tracks").arg(static_cast<int>(card.album().trackCount()));
        if (card.hasRadio()) return card.radio().description();
        return QString {};
    }
    return kstore::QGadgetListModel::data(index, role);
}
auto ForYouListModel::roleNames() const -> QHash<int, QByteArray> {
    auto roles = kstore::QGadgetListModel::roleNames();
    roles.insert(Qt::UserRole + 1024, "card");
    roles.insert(Qt::UserRole + 1025, "itemId");
    roles.insert(Qt::UserRole + 1026, "coverId");
    roles.insert(Qt::UserRole + 1027, "detail");
    return roles;
}

RadioQueueListModel::RadioQueueListModel(QObject* parent)
    : ItemIdListModel(this, parent, enums::ItemType::ItemRadioQueue) {}
auto RadioQueueListModel::data(const QModelIndex& index, int role) const -> QVariant {
    if (index.row() < 0 || index.row() >= rowCount()) return {};
    if (role == RadioRole) return QVariant::fromValue(at(index.row()));
    return ItemIdListModel::data(index, role);
}
auto RadioQueueListModel::roleNames() const -> QHash<int, QByteArray> {
    auto roles = ItemIdListModel::roleNames();
    roles.insert(RadioRole, "radio");
    return roles;
}

MixSongListModel::MixSongListModel(QObject* parent)
    : EntityList<model::Song>(parent,enums::ItemType::ItemSong,AppStore::instance()->songs),
      m_disc_count(1) {
    connect(&m_item, &EntityView::changed, this, &MixSongListModel::mixChanged);
}
MixSongListModel::~MixSongListModel() {}
auto MixSongListModel::mix() const -> mix_type { return m_item.item(); }
auto MixSongListModel::discCount() const -> qint32 { return m_disc_count; }
void MixSongListModel::setMixId(i64 id) { m_item.setItemId(id); }
void MixSongListModel::setDiscCount(qint32 c) {
    if (c != m_disc_count) {
        m_disc_count = c;
        discCountChanged();
    }
}

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
