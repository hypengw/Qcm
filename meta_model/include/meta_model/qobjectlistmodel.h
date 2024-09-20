#pragma once

#include <type_traits>

#include "meta_model/qgadget_helper.h"
#include "meta_model/qmetaobjectmodel.h"

namespace meta_model
{

class QObjectListModel : public QMetaListModel<QObject*, QObjectListModel> {
public:
    using base_type = QMetaListModel<QObject*, QObjectListModel>;

    QObjectListModel(const QMetaObject& meta, bool is_owner, QObject* parent = nullptr)
        : base_type(parent), m_is_owner(is_owner) {
        this->updateRoleNames(meta);
    }
    virtual ~QObjectListModel() {}

    template<typename Tin>
        requires std::convertible_to<typename std::iterator_traits<Tin>::value_type, QObject*>
    void insert_impl(std::size_t pos, Tin beg, Tin end) {
        if (m_is_owner) {
            auto it = beg;
            for (; it < end; it++) {
                (*it)->setParent(this);
            }
        }
        base_type::insert_impl(pos, beg, end);
    }

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (auto prop = this->propertyOfRole(role); prop) {
            return prop.value().read(this->at(index.row()));
        }
        return {};
    };

private:
    bool m_is_owner;
};

} // namespace meta_model