module;
#include "QExtra/macro_qt.hpp"
#ifdef Q_MOC_RUN
#    include "Qcm/model/store_item.moc"
#endif
export module qcm:model.store_item;
export import :msg;
import qextra;

export namespace qcm::model
{
class SongStoreItem : public EntityView {
    Q_OBJECT
    Q_PROPERTY(qcm::model::Song item READ item NOTIFY changed FINAL)
public:
    explicit SongStoreItem(QObject* parent = nullptr): EntityView(parent) {}
    auto item() const -> Song {
        if (auto entry = store()->find(itemId())) return entry->data.value<Song>();
        return {};
    }
    void setItemId(const msg::id::SongId& id) { EntityView::setItemId(ItemId(id)); }
};
class AlbumStoreItem : public EntityView {
    Q_OBJECT
    Q_PROPERTY(qcm::model::Album item READ item NOTIFY changed FINAL)
public:
    explicit AlbumStoreItem(QObject* parent = nullptr): EntityView(parent) {}
    auto item() const -> Album {
        if (auto entry = store()->find(itemId())) return entry->data.value<Album>();
        return {};
    }
    void setItemId(const msg::id::AlbumId& id) { EntityView::setItemId(ItemId(id)); }
};
class ArtistStoreItem : public EntityView {
    Q_OBJECT
    Q_PROPERTY(qcm::model::Artist item READ item NOTIFY changed FINAL)
public:
    explicit ArtistStoreItem(QObject* parent = nullptr): EntityView(parent) {}
    auto item() const -> Artist {
        if (auto entry = store()->find(itemId())) return entry->data.value<Artist>();
        return {};
    }
    void setItemId(const msg::id::ArtistId& id) { EntityView::setItemId(ItemId(id)); }
};
class MixStoreItem : public EntityView {
    Q_OBJECT
    Q_PROPERTY(qcm::model::Mix item READ item NOTIFY changed FINAL)
public:
    explicit MixStoreItem(QObject* parent = nullptr): EntityView(parent) {}
    auto item() const -> Mix {
        if (auto entry = store()->find(itemId())) return entry->data.value<Mix>();
        return {};
    }
    void setItemId(const msg::id::MixId& id) { EntityView::setItemId(ItemId(id)); }
};
} // namespace qcm::model
