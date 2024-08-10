#pragma once
#include <QQmlEngine>
#include "qcm_interface/export.h"

namespace qcm
{
namespace enums
{
Q_NAMESPACE
QML_ELEMENT

enum class QCM_INTERFACE_API ApiStatus
{
    Uninitialized,
    Querying,
    Finished,
    Error
};
Q_ENUM_NS(ApiStatus);

enum class QCM_INTERFACE_API IdType
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

enum class QCM_INTERFACE_API ToastFlag
{
    TFCloseable = 1,
    TFSave      = 1 << 1,
};
Q_ENUM_NS(ToastFlag);
Q_DECLARE_FLAGS(ToastFlags, ToastFlag);
Q_DECLARE_OPERATORS_FOR_FLAGS(ToastFlags);
} // namespace enums

} // namespace qcm