#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QMetaProperty>
#include <set>
#include <span>
#include <type_traits>

namespace meta_model
{

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

    void insert(int index, const TGadget& gad) {
        beginInsertRows({}, index, index);
        m_items.insert(index, gad);
        endInsertRows();
    }

    void remove(int index) {
        beginRemoveRows({}, index, index);
        m_items.remove(index);
        endRemoveRows();
    }

    void replace(int idx, const TGadget& gad) {
        m_items[idx] = gad;
        dataChanged(index(idx), index(idx));
    }

    void resetModel(QList<TGadget> items) {
        beginResetModel();
        m_items = std::move(items);
        endResetModel();
    }

    template<typename IdFn>
        requires std::convertible_to<std::invoke_result_t<IdFn, TGadget>, std::string>
    void convertModel(std::span<TGadget> items, IdFn&& id_fn) {
        std::set<std::string> id_set;
        for (auto& el : items) {
            id_set.insert(id_fn(el));
        }

        for (int i = 0; i < m_items.size();) {
            auto id = id_fn(m_items[i]);
            if (id_set.contains(id))
                i++;
            else
                remove(i);
        }

        for (std::size_t i = 0; i < items.size(); i++) {
            auto in = items[i];
            if ((std::size_t)rowCount() == i) {
                insert(i, in);
            } else {
                auto& out = m_items[i];
                if (id_fn(in) != id_fn(out))
                    insert(i, in);
                else if (in != out)
                    replace(i, in);
            }
        }

        for (int i = items.size(); i < m_items.size(); i++) {
            remove(i);
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

} // namespace meta_model
