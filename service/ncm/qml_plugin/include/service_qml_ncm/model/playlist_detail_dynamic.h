#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/playlist_detail_dynamic.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class PlaylistDetailDynamic : public QObject {
    Q_OBJECT
public:
    PlaylistDetailDynamic(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::PlaylistDetailDynamic;

    READ_PROPERTY(bool, subscribed, m_subscribed, infoChanged)
    READ_PROPERTY(qint32, bookedCount, m_bookedCount, infoChanged)
    READ_PROPERTY(qint32, playCount, m_playCount, infoChanged)
    READ_PROPERTY(qint32, commentCount, m_comment_count, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_subscribed, in.subscribed);
        convert(o.m_playCount, in.playCount);
        convert(o.m_comment_count, in.commentCount);
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<PlaylistDetailDynamic, ncm::api::PlaylistDetailDynamic>);

} // namespace model

using PlaylistDetailDynamicQuerier_base =
    ApiQuerier<ncm::api::PlaylistDetailDynamic, model::PlaylistDetailDynamic>;
class PlaylistDetailDynamicQuerier : public PlaylistDetailDynamicQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    PlaylistDetailDynamicQuerier(QObject* parent = nullptr)
        : PlaylistDetailDynamicQuerier_base(parent) {}

    FORWARD_PROPERTY(model::PlaylistId, itemId, id)
};
} // namespace qcm
