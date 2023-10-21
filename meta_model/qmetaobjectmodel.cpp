#include "meta_model/qmetaobjectmodel.h"

#include <QMetaProperty>

using namespace meta_model;

QMetaListModelBase::QMetaListModelBase(QObject* parent)
    : QAbstractListModel(parent), m_meta(staticMetaObject) {}
QMetaListModelBase::~QMetaListModelBase() {}

QHash<int, QByteArray> QMetaListModelBase::roleNames() const { return m_role_names; }

void QMetaListModelBase::updateRoleNames(const QMetaObject& meta) {
    if (meta.className() != m_meta.className()) {
        // or reset?
        Q_EMIT layoutAboutToBeChanged();

        auto roleIndex = Qt::UserRole + 1;
        for (auto i = 0; i < meta.propertyCount(); i++) {
            auto prop = meta.property(i);
            m_role_names.insert(roleIndex++, prop.name());
        }
        m_meta = meta;

        Q_EMIT layoutChanged();
    }
}

const QMetaObject& QMetaListModelBase::meta() const { return m_meta; }

std::optional<QMetaProperty> QMetaListModelBase::propertyOfRole(int role) const {
    if (auto prop_idx = m_meta.indexOfProperty(m_role_names.value(role).constData());
        prop_idx != -1) {
        return m_meta.property(prop_idx);
    }
    return std::nullopt;
}