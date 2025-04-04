#pragma once

#include <QQmlEngine>

#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/backend_msg.hpp"
#include "Qcm/query/query.hpp"

namespace qcm
{
class ArtistListModel
    : public meta_model::QGadgetListModel<model::Artist,
                                          meta_model::QMetaListStore::VectorWithMap> {
    Q_OBJECT
    using base_type =
        meta_model::QGadgetListModel<model::Artist, meta_model::QMetaListStore::VectorWithMap>;

    using value_type = model::Artist;

public:
    ArtistListModel(QObject* parent = nullptr);
};

class ArtistsQuery : public query::QueryList<ArtistListModel> {
    Q_OBJECT
    QML_ELEMENT
public:
    ArtistsQuery(QObject* parent = nullptr);
    void reload() override;
    void fetchMore(qint32) override;
};

} // namespace qcm::query
