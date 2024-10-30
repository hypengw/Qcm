#pragma once

#include "qcm_interface/model/album.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/model/song.h"
#include "qcm_interface/model/playlist.h"
#include "core/qstr_helper.h"

namespace qcm::query
{

struct Artist : model::Artist {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
};
struct Album : model::Album {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
    GADGET_PROPERTY_DEF(std::vector<model::ArtistRefer>, artists, artists)
    inline static const QString Select { uR"(
    %1,
    GROUP_CONCAT(artist.itemId) AS artistIds, 
    GROUP_CONCAT(artist.name) AS artistNames,
    GROUP_CONCAT(artist.picUrl) AS artistPicUrls
)"_s.arg(model::Album::Select) };
};

struct Song : model::Song {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_VALUE_TYPE(t_song)
public:
    GADGET_PROPERTY_DEF(model::AlbumRefer, album, album)
    GADGET_PROPERTY_DEF(std::vector<model::ArtistRefer>, artists, artists)

    inline static QString Select { uR"(
    %1,
    album.itemId,
    album.name,
    GROUP_CONCAT(artist.itemId) AS artistIds, 
    GROUP_CONCAT(artist.name) AS artistNames,
    GROUP_CONCAT(artist.picUrl) AS artistPicUrls
)"_s.arg(model::Song::Select) };
};
} // namespace qcm::query