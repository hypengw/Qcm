#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/playlist_list.h"

#include "meta_model/qgadgetlistmodel.h"

#include "core/log.h"

namespace ncm::qml
{
namespace model
{

class PlaylistList : public meta_model::QGadgetListModel<qcm::model::Mix> {
    Q_OBJECT
public:
    PlaylistList(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<qcm::model::Mix>(parent), m_has_more(true) {}
    using out_type = ncm::api_model::PlaylistList;

    void handle_output(const out_type& re, const auto& input) {
        if (input.offset != (int)rowCount()) {
            return;
        }
        auto in_ = convert_from<std::vector<qcm::model::Mix>>(re.playlists);
        for (auto& el : in_) {
            // remove query
            el.picUrl = el.picUrl.split('?').front();
        }
        insert(rowCount(), in_);
        m_has_more = re.more;
    }

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
} // namespace model

using PlaylistListQuerier_base = NcmApiQuery<ncm::api::PlaylistList, model::PlaylistList>;
class PlaylistListQuerier : public PlaylistListQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    PlaylistListQuerier(QObject* parent = nullptr): PlaylistListQuerier_base(parent) {}

    FORWARD_PROPERTY(QString, cat, cat)
    FORWARD_PROPERTY(qint32, offset, offset)
    FORWARD_PROPERTY(qint32, limit, limit)

public:
    void fetch_more(qint32 cur_count) override { set_offset(cur_count); }
};

} // namespace ncm::qml
