#pragma once

#include "meta_model/qgadgetlistmodel.h"
#include "Qcm/backend_msg.h"

namespace qcm
{

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
