#pragma once

#include "qcm_interface/model/album.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/model/song.h"

namespace qcm::query
{

struct Album : model::Album {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(std::vector<model::ArtistRefer>, artists, artists)
};

struct Song : model::Song {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(QString, albumName, albumName)
    GADGET_PROPERTY_DEF(std::vector<model::ArtistRefer>, artists, artists)
};
} // namespace qcm::query