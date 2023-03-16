#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/recommend_songs.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class RecommandSongs : public QObject {
    Q_OBJECT
public:
    RecommandSongs(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::RecommendSongs;

    READ_PROPERTY(std::vector<Song>, dailySongs, m_dailySongs, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        CONVERT_PROPERTY(o.m_dailySongs, in.data.dailySongs);
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<RecommandSongs, ncm::api::RecommendSongs>);

} // namespace model

using RecommandSongsQuerier_base = ApiQuerier<ncm::api::RecommendSongs, model::RecommandSongs>;
class RecommandSongsQuerier : public RecommandSongsQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    RecommandSongsQuerier(QObject* parent = nullptr): RecommandSongsQuerier_base(parent) {}
};
} // namespace qcm
