#pragma once

#include "qcm_interface/model.h"
#include "ncm/model.h"

DECLARE_CONVERT(QDateTime, ncm::model::Time)
DECLARE_CONVERT(qcm::model::Artist, ncm::model::Artist)
DECLARE_CONVERT(qcm::model::Artist, ncm::model::Song::Ar)
DECLARE_CONVERT(qcm::model::Album, ncm::model::Album)
DECLARE_CONVERT(qcm::model::Playlist, ncm::model::Playlist)
DECLARE_CONVERT(qcm::model::Song, ncm::model::Song)
DECLARE_CONVERT(qcm::model::User, ncm::model::User)
DECLARE_CONVERT(qcm::model::Comment, ncm::model::Comment)
DECLARE_CONVERT(qcm::model::Djradio, ncm::model::Djradio)
DECLARE_CONVERT(qcm::model::Song, ncm::model::SongB)
DECLARE_CONVERT(qcm::model::Program, ncm::model::Program)