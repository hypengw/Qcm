#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/recommend_resource.h"

#include "core/log.h"

template<>
template<>
inline qcm::model::Playlist To<qcm::model::Playlist>::from(const ncm::model::RecommendResourceItem& in) {
    qcm::model::Playlist o;
    convert(o.id, in.id);
    convert(o.name, in.name);
    convert(o.picUrl, in.picUrl);
    convert(o.playCount, in.playcount);
    convert(o.updateTime, in.createTime);
    return o;
};

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
        emit  infoChanged();
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
