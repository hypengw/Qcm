#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/recommend_songs.h"
#include "qcm_interface/model/query_model.h"
#include "meta_model/qgadgetlistmodel.h"
#include "core/log.h"

namespace qcm
{

namespace model
{

class RecommendSongs : public meta_model::QGadgetListModel<query::Song> {
    Q_OBJECT
public:
    RecommendSongs(QObject* parent = nullptr): meta_model::QGadgetListModel<query::Song>(parent) {}
    using out_type = ncm::api_model::RecommendSongs;
};

} // namespace model

using RecommendSongsQuery_base = ApiQuerier<ncm::api::RecommendSongs, model::RecommendSongs>;
class RecommendSongsQuery : public RecommendSongsQuery_base {
    Q_OBJECT
    QML_ELEMENT
public:
    RecommendSongsQuery(QObject* parent = nullptr): RecommendSongsQuery_base(parent) {}

    void handle_output(const out_type& in) override {
        auto view = std::views::transform(in.data.dailySongs, [](const auto& el) {
            qcm::query::Song s;
            auto             oper = qcm::query::SongOper(s);
            convert(oper, el);
            convert(
                s.sourceId,
                ncm::model::SpecialId { std::string(ncm::model::SpecialId_DailySongRecommend) });
            return s;
        });
        tdata()->resetModel(view);
    }
};
} // namespace qcm
