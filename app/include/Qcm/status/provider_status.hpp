#pragma once

#include "meta_model/qgadgetlistmodel.h"
#include "Qcm/backend_msg.h"

namespace qcm
{
class ProviderMetaStatusModel
    : public meta_model::QGadgetListModel<msg::model::ProviderMeta,
                                          meta_model::QMetaListStore::Map> {
    Q_OBJECT
    QML_ELEMENT

    using Model = msg::model::ProviderMeta;
    using Base  = meta_model::QGadgetListModel<Model, meta_model::QMetaListStore::Map>;

public:
    ProviderMetaStatusModel(QObject* parent);
    ~ProviderMetaStatusModel();

    auto hash(const Model& t) const noexcept -> usize override {
        return std::hash<QStringView> {}(t.typeName());
    }
};

class ProviderStatusModel : public meta_model::QGadgetListModel<msg::model::ProviderStatus,
                                                                meta_model::QMetaListStore::Map> {
    Q_OBJECT
    QML_ELEMENT
    using Base =
        meta_model::QGadgetListModel<msg::model::ProviderStatus, meta_model::QMetaListStore::Map>;

public:
    ProviderStatusModel(QObject* parent);
    ~ProviderStatusModel();

    auto hash(const msg::model::ProviderStatus& t) const noexcept -> usize override {
        return std::hash<QStringView> {}(t.id_proto());
    }
};

} // namespace qcm
