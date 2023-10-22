#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/recommend_resource.h"

#include "core/log.h"

DEFINE_CONVERT(qcm::model::Playlist, ncm::model::RecommendResourceItem) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.picUrl, in.picUrl);
    convert(out.playCount, in.playcount);
    convert(out.updateTime, in.createTime);
}

namespace qcm
{

namespace model
{

class RecommendResource : public QObject {
    Q_OBJECT
public:
    RecommendResource(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::RecommendResource;

    READ_PROPERTY(std::vector<Playlist>, dailyPlaylists, m_dailyPlaylists, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_dailyPlaylists, in.recommend);
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<RecommendResource, ncm::api::RecommendResource>);

} // namespace model

using RecommendResourceQuerier_base =
    ApiQuerier<ncm::api::RecommendResource, model::RecommendResource>;
class RecommendResourceQuerier : public RecommendResourceQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    RecommendResourceQuerier(QObject* parent = nullptr): RecommendResourceQuerier_base(parent) {}
};
} // namespace qcm
