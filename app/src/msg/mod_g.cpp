import qcm;

auto qcm::model::common_extra(model::ItemId id) -> QQmlPropertyMap* {
    return AppStore::instance()->extra(id);
}