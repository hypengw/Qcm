#pragma once

#include <memory_resource>

#include <QtQml/QQmlPropertyMap>
#include <QtQml/QJSEngine>

#include "meta_model/share_store.hpp"

namespace qcm
{

struct ShareStoreExt {


    using ptr = std::unique_ptr<QQmlPropertyMap, void (*)(QQmlPropertyMap*)>;
    ShareStoreExt()
        : extra(ptr(new QQmlPropertyMap(), [](QQmlPropertyMap* p) {
              p->deleteLater();
          })) {
        QJSEngine::setObjectOwnership(extra.get(), QJSEngine::ObjectOwnership::CppOwnership);
    }
    ptr extra;
};

template<typename T>
class ShareStore
    : public meta_model::ShareStore<T, std::pmr::polymorphic_allocator<T>, ShareStoreExt> {
public:
    using base_type = meta_model::ShareStore<T, std::pmr::polymorphic_allocator<T>, ShareStoreExt>;
    ShareStore(): base_type() {}
};

} // namespace qcm