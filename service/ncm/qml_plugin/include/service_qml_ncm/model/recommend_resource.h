#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/recommend_resource.h"

#include "core/log.h"

DEFINE_CONVERT(qcm::model::Mix, ncm::model::RecommendResourceItem) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.picUrl, in.picUrl);
    convert(out.playCount, in.playcount);
    convert(out.updateTime, in.createTime);
}

namespace ncm::qml
{

namespace model
{

class RecommendResource : public QObject {
    Q_OBJECT
public:
    RecommendResource(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::RecommendResource;

    READ_PROPERTY(std::vector<qcm::model::Mix>, dailyPlaylists, m_dailyPlaylists, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_dailyPlaylists, in.recommend);
        emit infoChanged();
    }

signals:
    void infoChanged();
};

} // namespace model

using RecommendResourceQuerier_base =
    NcmApiQuery<ncm::api::RecommendResource, model::RecommendResource>;
class RecommendResourceQuerier : public RecommendResourceQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    RecommendResourceQuerier(QObject* parent = nullptr): RecommendResourceQuerier_base(parent) {}
};
} // namespace ncm::qml
