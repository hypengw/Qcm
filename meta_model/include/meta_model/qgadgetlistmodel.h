#pragma once

#include <type_traits>

#include "meta_model/qmetaobjectmodel.h"

namespace meta_model
{

template<typename T>
concept cp_is_gadget = requires() { typename T::QtGadgetHelper; };
template<typename T>
concept cp_is_qobject = std::derived_from<T, QObject>;

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
