#pragma once
#include <QSqlQuery>
#include "qcm_interface/model/query_model.h"
namespace qcm::query
{
inline void load_query(model::Album& al, QSqlQuery& query, int& i) {
    al.id          = query.value(i++).toUrl();
    al.name        = query.value(i++).toString();
    al.picUrl      = query.value(i++).toString();
    al.trackCount  = query.value(i++).toInt();
    al.publishTime = query.value(i++).toDateTime();
}

inline void load_query(model::Song& song, QSqlQuery& query, int& i) {
    song.id       = query.value(i++).toUrl();
    song.name     = query.value(i++).toString();
    song.coverUrl = query.value(i++).toString();
    song.canPlay  = query.value(i++).toInt();
}

inline void load_query(model::Playlist& pl, QSqlQuery& query, int& i) {
    pl.id          = query.value(i++).toUrl();
    pl.name        = query.value(i++).toString();
    pl.picUrl      = query.value(i++).toString();
    pl.description = query.value(i++).toString();
    pl.trackCount  = query.value(i++).toInt();
    pl.playCount   = query.value(i++).toInt();
    pl.updateTime  = query.value(i++).toDateTime();
    pl.userId      = query.value(i++).toUrl();
}
inline void load_query(query::Song& self, QSqlQuery& query, int& i) {
    query::load_query((model::Song&)self, query, i);
    self.albumId    = query.value(i++).toUrl();
    self.album.id   = self.albumId;
    self.album.name = query.value(i++).toString();
    {
        auto artist_ids     = query.value(i++).toStringList();
        auto artist_names   = query.value(i++).toStringList();
        auto artist_picUrls = query.value(i++).toStringList();
        for (qsizetype i = 0; i < artist_ids.size(); i++) {
            auto& ar  = self.artists.emplace_back();
            ar.id     = artist_ids[i];
            ar.name   = artist_names[i];
            ar.picUrl = artist_picUrls[i];
        }
    }
}
inline void load_query(query::Album& self, QSqlQuery& query, int& i) {
    query::load_query((model::Album&)self, query, i);
    {
        auto artist_ids     = query.value(i++).toStringList();
        auto artist_names   = query.value(i++).toStringList();
        auto artist_picUrls = query.value(i++).toStringList();
        for (qsizetype i = 0; i < artist_ids.size(); i++) {
            auto& ar  = self.artists.emplace_back();
            ar.id     = artist_ids[i];
            ar.name   = artist_names[i];
            ar.picUrl = artist_picUrls[i];
        }
    }
}
} // namespace qcm::query