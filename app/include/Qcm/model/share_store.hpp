#pragma once

#include <memory_resource>

#include <QtQml/QQmlPropertyMap>
#include <QtQml/QJSEngine>

#include "kstore/share_store.hpp"
#include "core/core.h"

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
    : public kstore::ShareStore<T, std::pmr::polymorphic_allocator<T>, ShareStoreExt> {
public:
    using base_type = kstore::ShareStore<T, std::pmr::polymorphic_allocator<T>, ShareStoreExt>;
    ShareStore(): base_type() {}
};

} // namespace qcm