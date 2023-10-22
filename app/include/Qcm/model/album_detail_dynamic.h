#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/album_detail_dynamic.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class AlbumDetailDynamic : public QObject {
    Q_OBJECT
public:
    AlbumDetailDynamic(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::AlbumDetailDynamic;

    READ_PROPERTY(bool, isSub, m_is_sub, infoChanged)
    READ_PROPERTY(qint32, likedCount, m_like_count, infoChanged)
    READ_PROPERTY(qint32, commentCount, m_comment_count, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_is_sub, in.isSub);
        convert(o.m_like_count, in.likedCount);
        convert(o.m_comment_count, in.commentCount);
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<AlbumDetailDynamic, ncm::api::AlbumDetailDynamic>);

} // namespace model

using AlbumDetailDynamicQuerier_base = ApiQuerier<ncm::api::AlbumDetailDynamic, model::AlbumDetailDynamic>;
class AlbumDetailDynamicQuerier : public AlbumDetailDynamicQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumDetailDynamicQuerier(QObject* parent = nullptr): AlbumDetailDynamicQuerier_base(parent) {}

    FORWARD_PROPERTY(model::AlbumId, itemId, id)
};
}
