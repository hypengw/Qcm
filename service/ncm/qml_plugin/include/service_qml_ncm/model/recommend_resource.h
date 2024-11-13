#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/recommend_resource.h"
#include "meta_model/qgadgetlistmodel.h"

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

class RecommendResource : public meta_model::QGadgetListModel<qcm::model::Mix> {
    Q_OBJECT
public:
    RecommendResource(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<qcm::model::Mix>(parent) {}
    using out_type = ncm::api_model::RecommendResource;
};

} // namespace model

using RecommendResourceQuerier_base =
    NcmApiQuery<ncm::api::RecommendResource, model::RecommendResource>;
class RecommendResourceQuerier : public RecommendResourceQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    RecommendResourceQuerier(QObject* parent = nullptr): RecommendResourceQuerier_base(parent) {}

    void handle_output(const out_type& in) {
        auto                         view = std::views::transform(std::views::filter(in.recommend,
                                                             [](const auto& el) {
                                                                 return el.id !=
                                                                        ncm::model::RadarId_Private;
                                                             }),
                                          [](const auto& el) {
                                              qcm::model::Mix mix;
                                              convert(mix, el);
                                              return mix;
                                          });
        std::vector<qcm::model::Mix> mixs { view.begin(), view.end() };
        tdata()->resetModel(mixs);
    }
};
} // namespace ncm::qml
