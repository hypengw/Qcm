#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "core/log.h"
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/artist_albums.h"

#include "meta_model/qgadgetlistmodel.h"

namespace qcm
{
namespace model
{

class ArtistAlbums : public meta_model::QGadgetListModel<Album> {
    Q_OBJECT
public:
    ArtistAlbums(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<Album>(parent), m_has_more(true) {}
    using out_type = ncm::api_model::ArtistAlbums;

    void handle_output(const out_type& in, const auto& input) {
        if (input.offset != (int)rowCount()) {
            return;
        }
        if (! in.hotAlbums.empty()) {
            auto in_      = To<std::vector<model::Album>>::from(in.hotAlbums);
            auto in_count = in_.size();
            auto count    = rowCount();
            beginInsertRows({}, count, count + in_count - 1);
            for (auto& el : in_) {
                items().append(el);
            }
            endInsertRows();
        }
        m_has_more = in.more;
    }

    // list model override
    //
    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }
signals:
    void fetchMoreReq(qint32);

private:
    bool m_has_more;
};
static_assert(modelable<ArtistAlbums, ncm::api::ArtistAlbums>);
} // namespace model

using ArtistAlbumsQuerier_base = ApiQuerier<ncm::api::ArtistAlbums, model::ArtistAlbums>;
class ArtistAlbumsQuerier : public ArtistAlbumsQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    ArtistAlbumsQuerier(QObject* parent = nullptr): ArtistAlbumsQuerier_base(parent) {}

    FORWARD_PROPERTY(model::ArtistId, artistId, id)
    FORWARD_PROPERTY(qint32, offset, offset)
    FORWARD_PROPERTY(qint32, limit, limit)

public:
    void fetch_more(qint32 cur_count) override { set_offset(cur_count); }
};

} // namespace qcm
