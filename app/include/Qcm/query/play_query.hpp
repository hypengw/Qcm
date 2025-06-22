#pragma once

#include <QQmlEngine>
#include "Qcm/query/query.hpp"
#include "Qcm/model/store_item.hpp"
#include "Qcm/model/list_models.hpp"

namespace qcm
{
class PlayQuery : public Query {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    PlayQuery(QObject* parent = nullptr);
    void reload() override;

    auto          itemId() const -> model::ItemId;
    void          setItemId(model::ItemId);
    Q_SIGNAL void itemIdChanged(model::ItemId);

private:
    void          setSubQuery(Query* sub_query);
    Query*        m_sub_query;
    model::ItemId m_item_id;
};
} // namespace qcm