#include "Qcm/store.hpp"
#include "Qcm/util/global_static.hpp"
namespace qcm
{
AppStore::AppStore(QObject* parent): QObject(parent) {}
AppStore::~AppStore() {}
auto AppStore::instance() -> AppStore* {
    static auto the =
        GlobalStatic::instance()->add<AppStore>("store", new AppStore(nullptr), [](AppStore* p) {
            delete p;
        });
    return the;
}
AppStore* AppStore::create(QQmlEngine*, QJSEngine*) {
    auto self = instance();
    // not delete on qml
    QJSEngine::setObjectOwnership(self, QJSEngine::CppOwnership);
    return self;
}

auto AppStore::extra(model::ItemId item_id) const -> QQmlPropertyMap* {
    using ItemType = enums::ItemType;
    auto id        = item_id.id();
    switch (item_id.type()) {
    case ItemType::ItemAlbum: {
        if (auto extend = albums.query_extend(id)) {
            return extend->extra.get();
        }
        break;
    }
    case ItemType::ItemSong: {
        if (auto extend = songs.query_extend(id)) {
            return extend->extra.get();
        }
        break;
    }
    case ItemType::ItemArtist: {
        if (auto extend = artists.query_extend(id)) {
            return extend->extra.get();
        }
        break;
    }
    default: {
        break;
    }
    }
    return nullptr;
}

} // namespace qcm

#include <Qcm/moc_store.cpp>