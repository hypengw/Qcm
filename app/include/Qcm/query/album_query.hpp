#pragma once

#include <QQmlEngine>

#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/backend_msg.hpp"
#include "Qcm/query/query.hpp"

namespace qcm
{
class AlbumListModel
    : public meta_model::QGadgetListModel<msg::model::Album, meta_model::QMetaListStore::Share> {
    Q_OBJECT
    using base_type =
        meta_model::QGadgetListModel<msg::model::Album, meta_model::QMetaListStore::Share>;

    using value_type = msg::model::Album;

public:
    AlbumListModel(QObject* parent = nullptr);

    Q_INVOKABLE QQmlPropertyMap* extra(i32 idx) const;
};

class AlbumsQuery : public query::QueryList<AlbumListModel> {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumsQuery(QObject* parent = nullptr);
    void reload() override;
    void fetchMore(qint32) override;
};

} // namespace qcm
