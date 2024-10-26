#pragma once

#include <QSqlQuery>
#include "qcm_interface/model/album.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/model/song.h"
#include "qcm_interface/model/playlist.h"
#include "core/qstr_helper.h"

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

struct Artist : model::Artist {
    Q_GADGET
public:
};
struct Album : model::Album {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(std::vector<model::ArtistRefer>, artists, artists)
    inline static const QString Select { uR"(
    %1,
    GROUP_CONCAT(artist.itemId) AS artistIds, 
    GROUP_CONCAT(artist.name) AS artistNames,
    GROUP_CONCAT(artist.picUrl) AS artistPicUrls
)"_s.arg(model::Album::Select) };
    void                        load_query(QSqlQuery& query, int& i) {
        query::load_query(*this, query, i);
        {
            auto artist_ids     = query.value(i++).toStringList();
            auto artist_names   = query.value(i++).toStringList();
            auto artist_picUrls = query.value(i++).toStringList();
            for (qsizetype i = 0; i < artist_ids.size(); i++) {
                auto& ar  = this->artists.emplace_back();
                ar.id     = artist_ids[i];
                ar.name   = artist_names[i];
                ar.picUrl = artist_picUrls[i];
            }
        }
    }
};

struct Song : model::Song {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(QString, albumName, albumName)
    GADGET_PROPERTY_DEF(std::vector<model::ArtistRefer>, artists, artists)

    inline static QString Select { uR"(
    %1,
    album.itemId,
    album.name,
    GROUP_CONCAT(artist.itemId) AS artistIds, 
    GROUP_CONCAT(artist.name) AS artistNames,
    GROUP_CONCAT(artist.picUrl) AS artistPicUrls
)"_s.arg(model::Song::Select) };

    void load_query(QSqlQuery& query, int& i) {
        query::load_query(*this, query, i);
        albumId   = query.value(i++).toUrl();
        albumName = query.value(i++).toString();
        {
            auto artist_ids     = query.value(i++).toStringList();
            auto artist_names   = query.value(i++).toStringList();
            auto artist_picUrls = query.value(i++).toStringList();
            for (qsizetype i = 0; i < artist_ids.size(); i++) {
                auto& ar  = this->artists.emplace_back();
                ar.id     = artist_ids[i];
                ar.name   = artist_names[i];
                ar.picUrl = artist_picUrls[i];
            }
        }
    }
};
} // namespace qcm::query