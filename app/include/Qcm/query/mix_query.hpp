#pragma once

#include <QQmlEngine>

#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/query/query.hpp"
#include "Qcm/model/store_item.hpp"

namespace qcm
{
class MixListModel
    : public meta_model::QGadgetListModel<model::Mix,
                                          meta_model::QMetaListStore::VectorWithMap> {
    Q_OBJECT
    using base_type =
        meta_model::QGadgetListModel<model::Mix, meta_model::QMetaListStore::VectorWithMap>;

    using value_type = model::Mix;

public:
    MixListModel(QObject* parent = nullptr);
};

class MixesQuery : public query::QueryList<MixListModel> {
    Q_OBJECT
    QML_ELEMENT
public:
    MixesQuery(QObject* parent = nullptr);
    void reload() override;
    void fetchMore(qint32) override;
};

class MixQuery : public query::Query<model::MixStoreItem> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    MixQuery(QObject* parent = nullptr);
    void reload() override;

    auto itemId() const -> model::ItemId;
    void setItemId(model::ItemId);

    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};

} // namespace qcm
