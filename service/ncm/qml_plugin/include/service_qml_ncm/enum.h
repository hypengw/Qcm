#pragma once
#include "core/core.h"
#include "ncm/enum.h"

#include <QQmlEngine>

namespace ncm::qml
{
namespace enums
{
Q_NAMESPACE QML_ELEMENT

    enum class IdType {
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
} // namespace ncm::qml

STATIC_CAST_CONVERT(ncm::qml::enums::IdType, ncm::enums::IdType)