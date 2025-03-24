#pragma once

#include "meta_model/qgadget_list_model.hpp"
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
    ProviderMetaStatusModel(QObject* parent = nullptr);
    ~ProviderMetaStatusModel();
};

class ProviderStatusModel : public meta_model::QGadgetListModel<msg::model::ProviderStatus,
                                                                meta_model::QMetaListStore::Map> {
    Q_OBJECT
    QML_ELEMENT
    using Base =
        meta_model::QGadgetListModel<msg::model::ProviderStatus, meta_model::QMetaListStore::Map>;

public:
    ProviderStatusModel(QObject* parent = nullptr);
    ~ProviderStatusModel();
};

} // namespace qcm
