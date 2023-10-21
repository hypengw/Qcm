#pragma once

#include <type_traits>

#include "meta_model/qgadget_helper.h"
#include "meta_model/qmetaobjectmodel.h"

namespace meta_model
{

template<typename TGadget>
    requires cp_is_gadget<TGadget>
class QGadgetListModel : public QMetaListModel<TGadget> {
public:
    QGadgetListModel(QObject* parent = nullptr): QMetaListModel<TGadget>(parent) {
        this->updateRoleNames(TGadget::staticMetaObject);
    }
    virtual ~QGadgetListModel() {}

    // override
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (auto prop = this->propertyOfRole(role); prop) {
            return prop.value().readOnGadget(&this->item(index.row()));
        }
        return {};
    };
};

} // namespace meta_model
