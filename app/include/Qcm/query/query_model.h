#pragma once
#include <QSqlQuery>
#include "qcm_interface/model/query_model.h"
#include "qcm_interface/sql/meta_sql.h"
namespace qcm::query
{
using Converter = std::function<QVariant(const QVariant&)>;
auto get_from_converter(int id) -> std::optional<Converter>;
auto get_to_converter(int id) -> std::optional<Converter>;

template<typename T>
void load_query(QSqlQuery& query, T& gad, int& i) {
    for (auto prop_i : T::sql().idxs) {
        auto val  = query.value(i++);
        auto prop = T::staticMetaObject.property(prop_i);
        if (auto c = get_from_converter(prop.metaType().id())) {
            val = (*c)(val);
        }
        prop.writeOnGadget(&gad, val);
    }
}

template<>
inline void load_query<std::vector<model::ArtistRefer>>(QSqlQuery&                       query,
                                                        std::vector<model::ArtistRefer>& artists,
                                                        int&                             i) {
    auto artist_ids     = query.value(i++).toStringList();
    auto artist_names   = query.value(i++).toStringList();
    auto artist_picUrls = query.value(i++).toStringList();
    for (qsizetype i = 0; i < artist_ids.size(); i++) {
        auto& ar  = artists.emplace_back();
        ar.id     = artist_ids[i];
        ar.name   = artist_names[i];
        ar.picUrl = artist_picUrls[i];
    }
}

template<>
inline void load_query<Album>(QSqlQuery& query, Album& gad, int& i) {
    load_query<model::Album>(query, gad, i);
    load_query(query, gad.artists, i);
}

inline void load_query(model::Album& al, QSqlQuery& query, int& i) {
    al.staticMetaObject.property(i).metaType().id();
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

inline void load_query(model::Radio& dj, QSqlQuery& query, int& i) {
    dj.id           = query.value(i++).toUrl();
    dj.name         = query.value(i++).toString();
    dj.picUrl       = query.value(i++).toString();
    dj.description  = query.value(i++).toString();
    dj.programCount = query.value(i++).toInt();
}
inline void load_query(model::Program& dj, QSqlQuery& query, int& i) {
    dj.id           = query.value(i++).toUrl();
    dj.name         = query.value(i++).toString();
    dj.description  = query.value(i++).toString();
    dj.duration     = query.value(i++).toDateTime();
    dj.coverUrl     = query.value(i++).toString();
    dj.songId       = query.value(i++).toUrl();
    dj.createTime   = query.value(i++).toDateTime();
    dj.serialNumber = query.value(i++).toInt();
    dj.radioId      = query.value(i++).toUrl();
}
inline void load_query(query::Program& self, QSqlQuery& query, int& i) {
    query::load_query((model::Program&)self, query, i);
}

inline void load_query(query::Song& self, QSqlQuery& query, int& i) {
    query::load_query((model::Song&)self, query, i);
    self.albumId      = query.value(i++).toUrl();
    self.album.id     = self.albumId;
    self.album.name   = query.value(i++).toString();
    self.album.picUrl = query.value(i++).toString();
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