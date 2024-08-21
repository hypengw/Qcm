#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/recommend_songs.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class RecommendSongs : public QObject {
    Q_OBJECT
public:
    RecommendSongs(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::RecommendSongs;

    READ_PROPERTY(std::vector<Song>, dailySongs, m_dailySongs, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_dailySongs, in.data.dailySongs);
        for (auto& s : o.m_dailySongs) {
            convert(
                s.sourceId,
                ncm::model::SpecialId { std::string(ncm::model::SpecialId_DailySongRecommend) });
        }
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<RecommendSongs, ncm::api::RecommendSongs>);

} // namespace model

using RecommendSongsQuerier_base = ApiQuerier<ncm::api::RecommendSongs, model::RecommendSongs>;
class RecommendSongsQuerier : public RecommendSongsQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    RecommendSongsQuerier(QObject* parent = nullptr): RecommendSongsQuerier_base(parent) {}
};
} // namespace qcm
