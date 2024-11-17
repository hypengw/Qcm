#pragma once

#include "qcm_interface/model/album.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/model/song.h"
#include "qcm_interface/model/mix.h"
#include "qcm_interface/model/radio.h"
#include "qcm_interface/model/program.h"
#include "core/qstr_helper.h"

namespace qcm::query
{

struct Artist : qcm::model::Artist {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
};
struct Album : qcm::model::Album {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
    GADGET_PROPERTY_DEF(std::vector<qcm::model::ArtistRefer>, artists, artists)
    QCM_INTERFACE_API static auto sql() -> const model::ModelSql&;
};

struct Song : qcm::model::Song {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_VALUE_TYPE(t_song)
public:
    GADGET_PROPERTY_DEF(qcm::model::AlbumRefer, album, album)
    GADGET_PROPERTY_DEF(std::vector<model::ArtistRefer>, artists, artists)

    QCM_INTERFACE_API static auto sql() -> const model::ModelSql&;
};

struct Radio : qcm::model::Radio {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
};

struct Program : qcm::model::Program {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
};
} // namespace qcm::query