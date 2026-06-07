module;

#include <QtCore/QHash>
#include <QtCore/QVariant>

export module qcm:model.item_id_list;
export import :model.item_id;
import qextra;

export namespace qcm::model
{

class ItemIdListModel : public kstore::QGadgetListModel {
public:
    static constexpr int ItemIdRole = Qt::UserRole + 1024;

    template<typename T>
    ItemIdListModel(T* oper, QObject* parent, enums::ItemType item_type)
        : kstore::QGadgetListModel(oper, parent),
          m_item_type(item_type) {}

    auto data(const QModelIndex& index, int role = Qt::DisplayRole) const -> QVariant override {
        if (role == ItemIdRole) {
            auto key = m_oper->rawKeyAt(index.row());
            if (! key.isValid()) return {};
            return QVariant::fromValue(ItemId { m_item_type, key.toLongLong() });
        }
        return kstore::QGadgetListModel::data(index, role);
    }

    auto roleNames() const -> QHash<int, QByteArray> override {
        auto roles = kstore::QGadgetListModel::roleNames();
        roles.insert(ItemIdRole, "itemId");
        return roles;
    }

private:
    enums::ItemType m_item_type;
};

} // namespace qcm::model
