#pragma once

#include <QQmlEngine>

#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/backend_msg.h"
#include "Qcm/query/query.hpp"

namespace qcm
{
class AlbumListModel
    : public meta_model::QGadgetListModel<msg::model::Album,
                                          meta_model::QMetaListStore::VectorWithMap> {
    Q_OBJECT
    using base_type =
        meta_model::QGadgetListModel<msg::model::Album, meta_model::QMetaListStore::VectorWithMap>;

    using value_type = msg::model::Album;

public:
    AlbumListModel(QObject* parent = nullptr);
};

class AlbumsQuery : public query::QueryList<AlbumListModel> {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumsQuery(QObject* parent = nullptr);
    void reload() override;
};

} // namespace qcm
