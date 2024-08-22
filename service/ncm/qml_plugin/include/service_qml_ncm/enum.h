#pragma once
#include "core/core.h"
#include "ncm/enum.h"

#include <QQmlEngine>

namespace qcm::qml_ncm
{
namespace enums
{
Q_NAMESPACE QML_ELEMENT

enum class IdType
{
    IdTypeSong = 0,
    IdTypeProgram,
    IdTypeAlbum,
    IdTypePlaylist,
    IdTypeDjradio,
    IdTypeArtist,
    IdTypeUser,
    IdTypeComment,
    IdTypeSpecial,
};
Q_ENUM_NS(IdType)
} // namespace enums
} // namespace qcm::qml_ncm


STATIC_CAST_CONVERT(qcm::qml_ncm::enums::IdType, ncm::enums::IdType)