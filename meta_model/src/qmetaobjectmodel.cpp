#include "meta_model/qmeta_list_model.hpp"

#include <QMetaProperty>

namespace meta_model
{

namespace detail
{
QMetaListModelBase::QMetaListModelBase(QObject* parent)
    : QMetaModelBase<QAbstractListModel>(parent) {}
QMetaListModelBase::~QMetaListModelBase() {}
} // namespace detail

void detail::update_role_names(QHash<int, QByteArray>& role_names, const QMetaObject& meta) {
    role_names.clear();

    auto roleIndex = Qt::UserRole + 1;
    for (auto i = 0; i < meta.propertyCount(); i++) {
        auto prop = meta.property(i);
        role_names.insert(roleIndex++, prop.name());
    }
}

auto readOnGadget(const QVariant& obj, const char* name) -> QVariant {
    if (auto meta = obj.metaType().metaObject()) {
        if (auto p = meta->property(meta->indexOfProperty(name)); p.isValid()) {
            return p.readOnGadget(obj.constData());
        }
    }
    return {};
}
} // namespace meta_model