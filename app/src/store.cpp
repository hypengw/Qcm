#include "Qcm/store.hpp"
#include "qcm_interface/global_static.h"
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

auto AppStore::albumExtra(QString key) const -> QQmlPropertyMap* {
    auto key_id = key.toLongLong();

    if (auto extend = albums.query_extend(key_id)) {
        return extend->extra.get();
    }

    return nullptr;
}
auto AppStore::songExtra(QString key) const -> QQmlPropertyMap* {
    auto key_id = key.toLongLong();
    if (auto extend = songs.query_extend(key_id)) {
        return extend->extra.get();
    }
    return nullptr;
}

} // namespace qcm

#include <Qcm/moc_store.cpp>