#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QMetaProperty>

namespace meta_model
{

struct Test {
    Q_GADGET
    Q_PROPERTY(QString id MEMBER id)
    QString id;
};

template<typename T>
concept cp_is_gadget = requires() { typename T::QtGadgetHelper; };
template<typename T>
concept cp_is_qobject = std::derived_from<T, QObject>;

template<typename TGadget>
    requires cp_is_gadget<TGadget>
class QGadgetListModel : public QAbstractListModel {
public:
    QGadgetListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent), m_meta_object(TGadget::staticMetaObject) {
        auto& meta      = m_meta_object;
        auto  roleIndex = Qt::UserRole + 1;
        for (auto i = 0; i < meta.propertyCount(); i++) {
            auto prop = meta.property(i);
            // if (prop.hasNotifySignal())
            m_role_names.insert(roleIndex++, prop.name());
        }
    }
    // override
    int      rowCount(const QModelIndex& = QModelIndex()) const override { return m_items.size(); }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (auto prop_idx = m_meta_object.indexOfProperty(m_role_names.value(role).constData());
            prop_idx != -1) {
            return m_meta_object.property(prop_idx).readOnGadget(&m_items.at(index.row()));
        }
        return {};
    };
    QHash<int, QByteArray> roleNames() const override { return m_role_names; }

protected:
    QList<TGadget>& items() { return m_items; }

private:
    QList<TGadget>         m_items;
    QHash<int, QByteArray> m_role_names;

    QMetaObject m_meta_object;
};

using tt = QGadgetListModel<Test>;

} // namespace meta_model