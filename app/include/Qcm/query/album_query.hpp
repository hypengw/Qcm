#pragma once

#include <QQmlEngine>

#include "meta_model/qgadgetlistmodel.h"
#include "Qcm/backend_msg.h"
#include "qcm_interface/query.h"

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

    auto hash(const value_type&) const noexcept -> usize override;

    bool canFetchMore(const QModelIndex&) const override { return this->offset() <m_total; }
    void fetchMore(const QModelIndex&) override {
        emit fetchMoreReq(rowCount());
    }

    Q_SIGNAL void fetchMoreReq(qint32);

private:
    i64 m_total;
};

class AlbumsQuery : public query::QueryList<AlbumListModel> {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumsQuery(QObject* parent = nullptr);
    void reload() override;
};

} // namespace qcm
