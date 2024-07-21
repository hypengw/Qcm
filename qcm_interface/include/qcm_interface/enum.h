#pragma once
#include <QQmlEngine>

namespace qcm
{
namespace enums
{
Q_NAMESPACE
QML_ELEMENT

enum class ApiStatus
{
    Uninitialized,
    Querying,
    Finished,
    Error
};
Q_ENUM_NS(ApiStatus);

enum class IdType
{
    IdTypeUnknown,
    IdTypeSong,
    IdTypeAlbum,
    IdTypeArtist,
    IdTypePlaylist,
    IdTypeUser,
    IdTypeComment,
    IdTypeDjradio,
    IdTypeProgram
};
Q_ENUM_NS(IdType)

} // namespace enums

} // namespace qcm