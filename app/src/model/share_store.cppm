export module qcm:model.share_store;
export import qcm.core;
export import qextra;



namespace qcm
{

export struct ShareStoreExt {
    using ptr = up<QQmlPropertyMap, void (*)(QQmlPropertyMap*)>;
    ShareStoreExt()
        : extra(ptr(QQmlPropertyMap::create(), [](QQmlPropertyMap* p) {
              p->deleteLater();
          })) {
        QJSEngine::setObjectOwnership(extra.get(), QJSEngine::ObjectOwnership::CppOwnership);
    }
    ptr extra;
};

export template<typename T>
class ShareStore : public kstore::ShareStore<T, cppstd::pmr::polymorphic_allocator<T>, ShareStoreExt> {
public:
    using base_type = kstore::ShareStore<T, cppstd::pmr::polymorphic_allocator<T>, ShareStoreExt>;
    ShareStore(): base_type() {}
};

} // namespace qcm